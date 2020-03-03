// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QObject>
#include <memory>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
class IPeakFunction;
} // namespace API
} // namespace Mantid

/**
 * IFunctionWrapper is a wrapper for IFunction pointer which is a QObject
 * and can send and receive signals.
 */
class IFunctionWrapper : public QObject {
  Q_OBJECT
public:
  IFunctionWrapper() : m_function(), m_compositeFunction(), m_peakFunction() {}

  /// IFunction pointer
  std::shared_ptr<Mantid::API::IFunction> function() { return m_function; }
  std::shared_ptr<Mantid::API::CompositeFunction> compositeFunction() {
    return m_compositeFunction;
  }
  std::shared_ptr<Mantid::API::IPeakFunction> peakFunction() {
    return m_peakFunction;
  }

  /// Set a new function from a string
  void setFunction(const QString &name);
  /// Set a new function from a pointer
  void setFunction(std::shared_ptr<Mantid::API::IFunction> function);

private:
  /// Pointer to the function
  std::shared_ptr<Mantid::API::IFunction> m_function;
  std::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;
  std::shared_ptr<Mantid::API::IPeakFunction> m_peakFunction;
};
