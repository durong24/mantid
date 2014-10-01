#include "MantidQtCustomInterfaces/MultiDatasetFit.h"
#include "MantidQtMantidWidgets/FunctionBrowser.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"

#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>

#include <boost/make_shared.hpp>
#include <qwt_plot_curve.h>

#include <vector>
#include <algorithm>

namespace{
  const int wsColumn = 0;
  const int wsIndexColumn = 1;
}

namespace MantidQt
{
namespace CustomInterfaces
{

/*==========================================================================================*/
/*                              AddWorkspaceDialog                                          */
/*==========================================================================================*/
AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent):QDialog(parent)
{
  m_uiForm.setupUi(this);
  // populate the combo box with names of eligible workspaces
  QStringList workspaceNames;
  auto wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for(auto name = wsNames.begin(); name != wsNames.end(); ++name)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( *name );
    if ( ws )
    {
      workspaceNames << QString::fromStdString( *name );
    }
  }
  connect(m_uiForm.cbWorkspaceName,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(workspaceNameChanged(const QString&)));
  m_uiForm.cbWorkspaceName->addItems( workspaceNames );

  connect(m_uiForm.cbAllSpectra,SIGNAL(stateChanged(int)),this,SLOT(selectAllSpectra(int)));
}

/**
 * Slot. Reacts on change of workspace name in the selection combo box.
 * @param wsName :: Name of newly selected workspace.
 */
void AddWorkspaceDialog::workspaceNameChanged(const QString& wsName)
{
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( ws )
  {
    int maxValue = static_cast<int>(ws->getNumberHistograms()) - 1;
    if ( maxValue < 0 ) maxValue = 0;
    m_maxIndex = maxValue;
    if ( m_uiForm.cbAllSpectra->isChecked() )
    {
      m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    }
    else
    {
      m_uiForm.leWSIndices->clear();
    }
  }
  else
  {
    m_maxIndex = 0;
    m_uiForm.leWSIndices->clear();
    m_uiForm.cbAllSpectra->setChecked(false);
  }
}

/**
 * Slot. Called when "All Spectra" check box changes its state
 */
void AddWorkspaceDialog::selectAllSpectra(int state)
{
  if ( state == Qt::Checked )
  {
    m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    m_uiForm.leWSIndices->setEnabled(false);
  }
  else
  {
    m_uiForm.leWSIndices->setEnabled(true);
  }

}

/**
 * Called on close if selection accepted.
 */
void AddWorkspaceDialog::accept()
{
  m_workspaceName = m_uiForm.cbWorkspaceName->currentText();
  m_wsIndices.clear();
  QString indexInput = m_uiForm.leWSIndices->text();
  if ( !indexInput.isEmpty() )
  {
    auto validator = boost::make_shared<Mantid::Kernel::ArrayBoundedValidator<int>>(0,m_maxIndex);
    Mantid::Kernel::ArrayProperty<int> prop("Indices",validator);
    std::string err = prop.setValue( indexInput.toStdString() );
    if ( err.empty() )
    {
      m_wsIndices = prop;
    }
    else
    {
      QMessageBox::warning(this, "MantidPlot - Error", QString("Some of the indices are outside the allowed range [0,%1]").arg(m_maxIndex));
    }
  }
  QDialog::accept();
}

/**
 * Called on close if selection rejected.
 */
void AddWorkspaceDialog::reject()
{
  m_workspaceName.clear();
  m_wsIndices.clear();
  QDialog::reject();
}

/*==========================================================================================*/
/*                                DatasetPlotData                                           */
/*==========================================================================================*/

class DatasetPlotData
{
public:
  DatasetPlotData(const QString& wsName, int wsIndex);
  ~DatasetPlotData();
  void show(QwtPlot *plot);
  void hide();
private:
  void setData(QwtPlotCurve *curve, Mantid::API::MatrixWorkspace_sptr ws, int wsIndex);
  QwtPlotCurve *m_dataCurve;
};

DatasetPlotData::DatasetPlotData(const QString& wsName, int wsIndex):
  m_dataCurve(new QwtPlotCurve(wsName + QString(" (%1)").arg(wsIndex)))
{
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( !ws )
  {
    QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }
  if ( wsIndex >= ws->getNumberHistograms() )
  {
    QString mess = QString("Spectrum %1 doesn't exist in workspace %2").arg(wsIndex).arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }

  setData( m_dataCurve, ws, wsIndex);

}

DatasetPlotData::~DatasetPlotData()
{
  m_dataCurve->detach();
  delete m_dataCurve;
}

void DatasetPlotData::setData(QwtPlotCurve *curve, Mantid::API::MatrixWorkspace_sptr ws, int wsIndex)
{
  std::vector<double> xValues = ws->readX(wsIndex);
  if ( ws->isHistogramData() )
  {
    auto xend = xValues.end() - 1;
    for(auto x = xValues.begin(); x != xend; ++x)
    {
      *x = (*x + *(x+1))/2;
    }
    xValues.pop_back();
  }
  m_dataCurve->setData( xValues.data(), ws->readY(wsIndex).data(), static_cast<int>(xValues.size()) );
}

void DatasetPlotData::show(QwtPlot *plot)
{
  m_dataCurve->attach(plot);
}

void DatasetPlotData::hide()
{
  m_dataCurve->detach();
}

/*==========================================================================================*/
/*                                PlotController                                            */
/*==========================================================================================*/

PlotController::PlotController(QObject *parent,QwtPlot *plot, QTableWidget *table, QComboBox *plotSelector, QPushButton *prev, QPushButton *next):
  QObject(parent),m_plot(plot),m_table(table),m_plotSelector(plotSelector),m_prevPlot(prev),m_nextPlot(next),m_currentIndex(-1)
{
  connect(parent,SIGNAL(dataTableUpdated()),this,SLOT(tableUpdated()));
  connect(prev,SIGNAL(clicked()),this,SLOT(prevPlot()));
  connect(next,SIGNAL(clicked()),this,SLOT(nextPlot()));
  connect(plotSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(plotDataSet(int)));
}

PlotController::~PlotController()
{
  std::cerr << "Plot controller destroyed." << std::endl;
  m_plotData.clear();
}

/**
 * Slot. Respond to changes in the data table.
 */
void PlotController::tableUpdated()
{
  m_plotSelector->blockSignals(true);
  m_plotSelector->clear();
  int rowCount = m_table->rowCount();
  for(int row = 0; row < rowCount; ++row)
  {
    QString itemText = QString("%1 (%2)").arg(m_table->item(row,wsColumn)->text(),m_table->item(row,wsIndexColumn)->text());
    m_plotSelector->insertItem( itemText );
  }
  m_plotData.clear();
  m_currentIndex = -1;
  m_plotSelector->blockSignals(false);
  plotDataSet( m_plotSelector->currentIndex() );
}

/**
 * Display the previous plot if there is one.
 */
void PlotController::prevPlot()
{
  int index = m_plotSelector->currentIndex();
  if ( index > 0 )
  {
    --index;
    m_plotSelector->setCurrentIndex( index );
  }
}

/**
 * Display the next plot if there is one.
 */
void PlotController::nextPlot()
{
  int index = m_plotSelector->currentIndex();
  if ( index < m_plotSelector->count() - 1 )
  {
    ++index;
    m_plotSelector->setCurrentIndex( index );
  }
}

/**
 * Plot a data set.
 * @param index :: Index (row) of the data set in the table.
 */
void PlotController::plotDataSet(int index)
{
  if ( index < 0 || index >= m_table->rowCount() ) return;
  if ( !m_plotData.contains(index) )
  {
    QString wsName = m_table->item( index, wsColumn )->text();
    int wsIndex = m_table->item( index, wsIndexColumn )->text().toInt();
    auto value = boost::make_shared<DatasetPlotData>( wsName, wsIndex );
    m_plotData.insert(index, value );
  }
  if ( m_currentIndex > -1 ) 
  {
    m_plotData[m_currentIndex]->hide();
  }
  m_plotData[index]->show( m_plot );
  m_plot->replot();
  m_currentIndex = index;
}

void PlotController::clear()
{
  m_plotData.clear();
}
/*==========================================================================================*/
/*                                MultiDatasetFit                                           */
/*==========================================================================================*/

//Register the class with the factory
DECLARE_SUBWINDOW(MultiDatasetFit);

/**
 * Constructor
 * @param parent :: The parent widget
 */
MultiDatasetFit::MultiDatasetFit(QWidget *parent)
:UserSubWindow(parent)
{
}

MultiDatasetFit::~MultiDatasetFit()
{
  m_plotController->clear();
}

/**
 * Initilize the layout.
 */
void MultiDatasetFit::initLayout()
{
  m_uiForm.setupUi(this);
  m_uiForm.hSplitter->setStretchFactor(0,0);
  m_uiForm.hSplitter->setStretchFactor(1,1);
  m_uiForm.vSplitter->setStretchFactor(0,0);
  m_uiForm.vSplitter->setStretchFactor(1,1);

  QHeaderView *header = m_uiForm.dataTable->horizontalHeader();
  header->setResizeMode(0,QHeaderView::Stretch);
  header->setResizeMode(1,QHeaderView::Fixed);

  m_uiForm.btnRemove->setEnabled( false );

  connect(m_uiForm.btnAddWorkspace,SIGNAL(clicked()),this,SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove,SIGNAL(clicked()),this,SLOT(removeSelectedSpectra()));
  connect(m_uiForm.dataTable,SIGNAL(itemSelectionChanged()), this,SLOT(workspaceSelectionChanged()));
  connect(m_uiForm.btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  m_plotController = new PlotController(this,
                                        m_uiForm.plot,
                                        m_uiForm.dataTable,
                                        m_uiForm.cbPlotSelector,
                                        m_uiForm.btnPrev,
                                        m_uiForm.btnNext);

  m_functionBrowser = new MantidQt::MantidWidgets::FunctionBrowser(NULL, true);
  m_uiForm.browserLayout->addWidget( m_functionBrowser );
}

/**
 * Show a dialog to select a workspace.
 */
void MultiDatasetFit::addWorkspace()
{
  AddWorkspaceDialog dialog(this);
  if ( dialog.exec() == QDialog::Accepted )
  {
    QString wsName = dialog.workspaceName().stripWhiteSpace();
    if ( Mantid::API::AnalysisDataService::Instance().doesExist( wsName.toStdString()) )
    {
      auto indices = dialog.workspaceIndices();
      for(auto i = indices.begin(); i != indices.end(); ++i)
      {
        addWorkspaceSpectrum( wsName, *i );
      }
      emit dataTableUpdated();
    }
    else
    {
      QMessageBox::warning(this,"MantidPlot - Warning",QString("Workspace \"%1\" doesn't exist.").arg(wsName));
    }
  }
}

/**
 * Add a spectrum from a workspace to the table.
 * @param wsName :: Name of a workspace.
 * @param wsIndex :: Index of a spectrum in the workspace (workspace index).
 */
void MultiDatasetFit::addWorkspaceSpectrum(const QString &wsName, int wsIndex)
{
  int row = m_uiForm.dataTable->rowCount();
  m_uiForm.dataTable->insertRow(row);

  auto cell = new QTableWidgetItem( wsName );
  m_uiForm.dataTable->setItem( row, wsColumn, cell );
  cell = new QTableWidgetItem( QString::number(wsIndex) );
  m_uiForm.dataTable->setItem( row, wsIndexColumn, cell );
}

/**
 * Slot. Called when selection in the data table changes.
 */
void MultiDatasetFit::workspaceSelectionChanged()
{
  auto selection = m_uiForm.dataTable->selectionModel();
  bool enableRemoveButton = selection->hasSelection();
  if ( enableRemoveButton )
  {
    enableRemoveButton = selection->selectedRows().size() > 0;
  }

  m_uiForm.btnRemove->setEnabled( enableRemoveButton );
}

/**
 * Slot. Called when "Remove" button is pressed.
 */
void MultiDatasetFit::removeSelectedSpectra()
{
  auto ranges = m_uiForm.dataTable->selectedRanges();
  if ( ranges.isEmpty() ) return;
  std::vector<int> rows;
  for(auto range = ranges.begin(); range != ranges.end(); ++range)
  {
    for(int row = range->topRow(); row <= range->bottomRow(); ++row)
    {
      rows.push_back( row );
    }
  }
  std::sort( rows.begin(), rows.end() );
  for(auto row = rows.rbegin(); row != rows.rend(); ++row)
  {
    m_uiForm.dataTable->removeRow( *row );
  }
  emit dataTableUpdated();
}

/**
 * Create a multi-domain function to fit all the spectra in the data table.
 */
boost::shared_ptr<Mantid::API::IFunction> MultiDatasetFit::createFunction() const
{
  // number of spectra to fit == size of the multi-domain function
  size_t nOfDataSets = static_cast<size_t>( m_uiForm.dataTable->rowCount() );
  if ( nOfDataSets == 0 )
  {
    throw std::runtime_error("There are no data sets specified.");
  }

  // description of a single function
  QString funStr = m_functionBrowser->getFunctionString();

  if ( nOfDataSets == 1 )
  {
    return Mantid::API::FunctionFactory::Instance().createInitialized( funStr.toStdString() );
  }

  QString multiFunStr = "composite=MultiDomainFunction,NumDeriv=1";
  funStr = ";(" + funStr + ")";
  for(size_t i = 0; i < nOfDataSets; ++i)
  {
    multiFunStr += funStr;
  }

  QStringList globals = m_functionBrowser->getGlobalParameters();
  QString globalTies;
  if ( !globals.isEmpty() )
  {
    globalTies = "ties=(";
    bool isFirst = true;
    foreach(QString par, globals)
    {
      if ( !isFirst ) globalTies += ",";
      else
        isFirst = false;

      for(size_t i = 1; i < nOfDataSets; ++i)
      {
        globalTies += QString("f%1.").arg(i) + par + "=";
      }
      globalTies += QString("f0.%1").arg(par);
    }
    globalTies += ")";
    multiFunStr += ";" + globalTies;
  }
  std::cerr << globalTies.toStdString() << std::endl;

  // create the multi-domain function
  auto fun = Mantid::API::FunctionFactory::Instance().createInitialized( multiFunStr.toStdString() );
  boost::shared_ptr<Mantid::API::MultiDomainFunction> multiFun = boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>( fun );
  if ( !multiFun )
  {
    throw std::runtime_error("Failed to create the MultiDomainFunction");
  }
  
  // set the domain indices
  for(size_t i = 0; i < nOfDataSets; ++i)
  {
    multiFun->setDomainIndex(i,i);
  }
  assert( multiFun->nFunctions() == nOfDataSets );
  return fun;
}

/**
 * Run the fitting algorithm.
 */
void MultiDatasetFit::fit()
{
  std::cerr << createFunction()->asString() << std::endl;
}

/*==========================================================================================*/
} // CustomInterfaces
} // MantidQt
