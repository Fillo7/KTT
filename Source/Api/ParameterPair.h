/** @file ParameterPair.h
  * Value for one kernel parameter.
  */
#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <KttPlatform.h>

namespace ktt
{

/** @class ParameterPair
  * Class which holds single value for one kernel parameter.
  */
class KTT_API ParameterPair
{
public:
    /** @fn ParameterPair()
      * Default constructor, creates parameter pair with empty name and value set to integer zero.
      */
    ParameterPair();

    /** @fn explicit ParameterPair(const std::string& name, const uint64_t value)
      * Constructor which creates parameter pair for integer parameter.
      * @param name Name of a parameter.
      * @param value Value of a parameter.
      */
    explicit ParameterPair(const std::string& name, const uint64_t value);

    /** @fn explicit ParameterPair(const std::string& name, const double value)
      * Constructor which creates parameter pair for floating-point parameter.
      * @param name Name of a parameter.
      * @param value Value of a parameter.
      */
    explicit ParameterPair(const std::string& name, const double value);

    /** @fn void SetValue(const uint64_t value)
      * Setter for value of an integer parameter.
      * @param value New value of an integer parameter.
      */
    void SetValue(const uint64_t value);

    /** @fn void SetValue(const double value)
      * Setter for value of a floating-point parameter.
      * @param value New value of a floating-point parameter.
      */
    void SetValue(const double value);

    /** @fn const std::string& GetName() const
      * Returns name of a parameter.
      * @return Name of a parameter.
      */
    const std::string& GetName() const;

    /** @fn std::string GetString() const
      * Converts parameter pair to string.
      * @return String in format "parameterName: parameterValue".
      */
    std::string GetString() const;

    /** @fn uint64_t GetValue() const
      * Returns integer representation of parameter value.
      * @return Integer representation of parameter value.
      */
    uint64_t GetValue() const;

    /** @fn double GetValueDouble() const
      * Returns floating-point representation of parameter value.
      * @return Floating-point representation of parameter value.
      */
    double GetValueDouble() const;

    /** @fn bool HasValueDouble() const
      * Checks if parameter value was specified as floating-point.
      * @return True if parameter value was specified as floating-point, false otherwise.
      */
    bool HasValueDouble() const;

private:
    std::string m_Name;
    std::variant<uint64_t, double> m_Value;
};

} // namespace ktt