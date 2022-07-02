/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef IPCATALOG_H
#define IPCATALOG_H

namespace FOEDAG {

class Value {
 public:
  enum class Type { ConstInt, ParamInt, ParamString };
  Value() {}
  virtual ~Value() {}
  virtual uint32_t GetValue() const = 0;
  virtual const std::string GetSValue() const = 0;
  virtual const std::string& Name() const = 0;
  virtual Type GetType() const = 0;
};

class Constant : public Value {
 public:
  Constant(uint32_t value) : m_value(value) {}
  ~Constant() {}
  uint32_t GetValue() const { return m_value; }
  const std::string GetSValue() const { return std::to_string(m_value); }
  void SetValue(uint32_t value) { m_value = value; }
  const std::string& Name() const { return m_name; }
  Type GetType() const { return Type::ConstInt; }

 private:
  static std::string m_name;
  uint32_t m_value = 0;
};

class Parameter : public Value {
 public:
  Parameter(const std::string& name, uint32_t default_val)
      : m_name(name), m_default(default_val) {}
  ~Parameter() {}
  uint32_t GetValue() const { return (m_useDefault) ? m_default : m_value; }
  const std::string GetSValue() const { return std::to_string(m_value); }
  void SetValue(uint32_t value) {
    m_value = value;
    m_useDefault = false;
  }
  const std::string& Name() const { return m_name; }
  Type GetType() const { return Type::ParamInt; }

 private:
  std::string m_name;
  uint32_t m_default = 0;
  bool m_useDefault = true;
  uint32_t m_value = 0;
};

class SParameter : public Value {
 public:
  SParameter(const std::string& name, const std::string& default_val)
      : m_name(name), m_default(default_val) {}
  ~SParameter() {}
  uint32_t GetValue() const {
    return (m_useDefault) ? std::strtoull(m_default.c_str(), nullptr, 10)
                          : std::strtoull(m_value.c_str(), nullptr, 10);
  }
  const std::string GetSValue() const {
    return (m_useDefault) ? m_default : m_value;
  }
  void SetValue(const std::string& value) {
    m_value = value;
    m_useDefault = false;
  }
  const std::string& Name() const { return m_name; }
  Type GetType() const { return Type::ParamString; }

 private:
  std::string m_name;
  std::string m_default = 0;
  bool m_useDefault = true;
  std::string m_value = 0;
};

class Range {
 public:
  Range(Value* lrange, Value* rrange) : m_lrange(lrange), m_rrange(rrange) {}
  ~Range() {}
  const Value* LRange() const { return m_lrange; }
  const Value* RRange() const { return m_rrange; }

 private:
  Value* m_lrange = nullptr;
  Value* m_rrange = nullptr;
};

class Connector {
 public:
  enum class Type { Port, Interface };
  Connector() {}
  ~Connector() {}
  virtual Type GetType() const = 0;
};

class Port : public Connector {
 public:
  enum class Direction { Input, Output, Inout };
  enum class Function {
    Clock,
    AsyncSet,
    AsyncReset,
    SyncSet,
    SyncReset,
    Enable,
    Data
  };
  enum class Polarity { High, Low };

  Port(const std::string& name, Direction direction, Function function,
       Polarity polarity, Range& range)
      : m_name(name),
        m_direction(direction),
        m_function(function),
        m_polarity(polarity),
        m_range(range){};
  ~Port() {}
  Type GetType() const { return Type::Port; }
  const std::string& Name() const { return m_name; }
  Direction GetDirection() const { return m_direction; }
  Function GetFunction() const { return m_function; }
  Polarity GetPolarity() const { return m_polarity; }
  Range GetRange() const { return m_range; }

 private:
  std::string m_name;
  Direction m_direction;
  Function m_function;
  Polarity m_polarity;
  Range m_range;
};

class Interface : public Connector {
 public:
  Interface(const std::string& name, const std::vector<Connector*>& connections)
      : m_name(name), m_connections(connections) {}
  ~Interface() {}
  Type GetType() const { return Type::Interface; }
  const std::string& Name() const { return m_name; }
  const std::vector<Connector*>& Connections() const { return m_connections; }

 private:
  std::string m_name;
  std::vector<Connector*> m_connections;
};

class IPDefinition {
 public:
  enum class IPType { LiteXGenerator, Other };
  IPDefinition(IPType type, const std::string& name,
               const std::filesystem::path& filePath,
               const std::vector<Connector*>& connections,
               const std::vector<Value*>& parameters)
      : m_type(type),
        m_name(name),
        m_filePath(filePath),
        m_connections(connections),
        m_parameters(parameters){};
  ~IPDefinition() {}
  IPType Type() const { return m_type; }
  const std::string& Name() const { return m_name; }
  const std::vector<Connector*>& Connections() const { return m_connections; }
  const std::filesystem::path FilePath() const { return m_filePath; }
  const std::vector<Value*> Parameters() const { return m_parameters; }

 private:
  IPType m_type;
  std::string m_name;
  std::filesystem::path m_filePath;
  std::vector<Connector*> m_connections;
  std::vector<Value*> m_parameters;
};

class IPInstance {
 public:
  IPInstance(const std::string& ipname, const std::string& version,
             IPDefinition* definition, std::vector<Parameter>& parameters,
             const std::string& moduleName,
             const std::filesystem::path& outputFile)
      : m_ipname(ipname),
        m_version(version),
        m_definition(definition),
        m_parameters(parameters),
        m_moduleName(moduleName),
        m_outputFile(outputFile) {}
  ~IPInstance() {}
  const std::string& IPName() { return m_ipname; }
  const std::string& Version() { return m_version; }
  const IPDefinition* Definition() { return m_definition; }
  const std::vector<Parameter>& Parameters() { return m_parameters; }
  const std::string& ModuleName() { return m_moduleName; }
  const std::filesystem::path OutputFile() { return m_outputFile; }

 private:
  std::string m_ipname;
  std::string m_version;
  IPDefinition* m_definition;
  std::vector<Parameter> m_parameters;
  std::string m_moduleName;
  std::filesystem::path m_outputFile;
};

class IPCatalog {
 public:
  IPCatalog() {}
  virtual ~IPCatalog() {}
  bool addIP(IPDefinition* def);
  const std::vector<IPDefinition*>& Definitions() { return m_definitions; }
  IPDefinition* Definition(const std::string& name);
  void WriteCatalog(std::ostream& out);

 protected:
  std::vector<IPDefinition*> m_definitions;
  std::map<std::string, IPDefinition*> m_definitionMap;
};

}  // namespace FOEDAG

#endif
