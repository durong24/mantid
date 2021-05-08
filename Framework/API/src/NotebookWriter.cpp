// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/NotebookWriter.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

namespace Mantid {
namespace API {

namespace {
Mantid::Kernel::Logger g_log("NotebookWriter");
}

NotebookWriter::NotebookWriter() : m_cell_buffer(Json::arrayValue) {
  // Add header comments and code
  headerComment();
  headerCode();
}

/**
 * Add a code cell to the buffer of cells to write to the notebook
 *
 * @param array_code :: Json array of strings containing python code
 * for the code cell
 */
void NotebookWriter::codeCell(Json::Value array_code) {

  Json::Value cell_data;
  const Json::Value empty = Json::Value(Json::ValueType::objectValue);

  cell_data["cell_type"] = "code";
  cell_data["collapsed"] = false;
  cell_data["input"] = std::move(array_code);
  cell_data["language"] = "python";
  cell_data["metadata"] = empty;
  cell_data["outputs"] = Json::Value(Json::arrayValue);

  m_cell_buffer.append(cell_data);
}

/**
 * Add a code cell to the buffer of cells to write to the notebook
 *
 * @param string_code :: string containing the python for the code cell
 */
std::string NotebookWriter::codeCell(const std::string &string_code) {

  Json::Value cell_data;
  const Json::Value empty = Json::Value(Json::ValueType::objectValue);

  cell_data["cell_type"] = "code";
  cell_data["collapsed"] = false;
  cell_data["input"] = string_code;
  cell_data["language"] = "python";
  cell_data["metadata"] = empty;
  cell_data["outputs"] = Json::Value(Json::arrayValue);

  m_cell_buffer.append(cell_data);
  Json::StyledWriter writer;
  return writer.write(cell_data);
}

/**
 * Add a markdown cell to the buffer of cells to write to the notebook
 *
 * @param string_array :: json array of strings containing the python
 * code for the code cell
 */
void NotebookWriter::markdownCell(Json::Value string_array) {

  Json::Value cell_data;
  const Json::Value empty = Json::Value(Json::ValueType::objectValue);

  cell_data["cell_type"] = "markdown";
  cell_data["metadata"] = empty;
  cell_data["source"] = std::move(string_array);

  m_cell_buffer.append(cell_data);
}

/**
 * Add a markdown cell to the buffer of cells to write to the notebook
 *
 * @param string_text :: string containing the python code for the code cell
 */
std::string NotebookWriter::markdownCell(const std::string &string_text) {

  Json::Value cell_data;
  const Json::Value empty = Json::Value(Json::ValueType::objectValue);

  cell_data["cell_type"] = "markdown";
  cell_data["metadata"] = empty;
  cell_data["source"] = string_text;

  m_cell_buffer.append(cell_data);
  Json::StyledWriter writer;
  return writer.write(cell_data);
}

/**
 * Add a markdown cell of information for the user to the buffer of cells to
 * write to the notebook
 */
void NotebookWriter::headerComment() {

  Json::Value strings(Json::arrayValue);
  strings.append(Json::Value("This IPython Notebook was automatically "
                             "generated by MantidPlot, version: "));
  strings.append(Json::Value(Mantid::Kernel::MantidVersion::version()));
  strings.append(Json::Value("\n"));
  strings.append(Json::Value(Mantid::Kernel::MantidVersion::releaseNotes()));
  strings.append(Json::Value("\n\nThe following information may be useful:\n"
                             "* [Mantid Framework Python API Reference]"
                             "(http://docs.mantidproject.org/nightly/api/python/index.html)\n"
                             "* [IPython Notebook "
                             "Documentation](http://ipython.org/ipython-doc/stable/notebook/)\n"
                             "* [matplotlib Documentation](http://matplotlib.org/contents.html)\n\n"
                             "Help requests and bug reports should be submitted to the [Mantid forum.]"
                             "(http://forum.mantidproject.org)"));

  markdownCell(strings);
}

/**
 * Add code cells to the buffer of cells to write to the notebook
 * These are to import Mantid and matplotlib, and to warn the
 * user if the version of Mantid being used does not match the
 * version which generated the notebook.
 */
void NotebookWriter::headerCode() {

  Json::Value import_mantid(Json::arrayValue);

  import_mantid.append(Json::Value("#Import Mantid's Python API and IPython plotting tools\n"
                                   "from mantid.simpleapi import *\n"
                                   "from MantidIPython import *\n"
                                   "\n"
                                   "#Some magic to tell matplotlib how to behave in IPython Notebook. Use "
                                   "'%matplotlib nbagg' for interactive plots, if available.\n"
                                   "%matplotlib inline"));

  codeCell(import_mantid);
}

/**
 * Create a Json value containing the whole notebook
 *@return a Json value containing the whole notebook
 */
Json::Value NotebookWriter::buildNotebook() {

  Json::Value output;
  const Json::Value empty = Json::Value(Json::ValueType::objectValue);

  Json::Value worksheet;
  worksheet["cells"] = m_cell_buffer;
  worksheet["metadata"] = empty;

  Json::Value worksheet_arr(Json::arrayValue);
  worksheet_arr.append(worksheet);

  Json::Value meta_name;
  meta_name["name"] = "Mantid Notebook";
  output["metadata"] = meta_name;
  output["nbformat"] = 3;
  output["nbformat_minor"] = 0;
  output["worksheets"] = worksheet_arr;

  return output;
}

/**
 * Create a formatted string of Json which describes a notebook
 * @return a formatted string of the Json which describes
 * the whole notebook
 */
std::string NotebookWriter::writeNotebook() {

  const Json::Value root = buildNotebook();

  Json::StyledWriter writer;
  std::string output_string = writer.write(root);

  return output_string;
}
} // namespace API
} // namespace Mantid
