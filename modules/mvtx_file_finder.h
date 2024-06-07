// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef MVTXFILEFINDER_H
#define MVTXFILEFINDER_H
/*
class to find all prdf files for each mvtx felix server
*/
#include <string>
#include <vector>
#include <map>

class mvtx_file_finder
{
 public:

  mvtx_file_finder(const std::string &name = "mvtx_file_finder") : m_name(name) {}
  ~mvtx_file_finder()
  {
    if (!m_file_list.empty())
    {
      m_file_list.clear();
    }
    m_valid_runtypes.clear();
  }
  
  void SetDataPath(const std::string& path) { m_data_path = path; }
  std::string GetDataPath() const { return m_data_path; }

  void SetRunNumber(const int run) { m_run_number = run; }
  void SetRunNumber(const std::string &run) { m_run_number = std::stoi(run); }
  int GetRunNumber() const { return m_run_number; }
  std::string GetRunNumberStr(){ 
        if (m_run_number_str == "0") FormatRunNumber();
        return m_run_number_str; 
    }

  void SetRunType(const std::string &runtype) { m_run_type = runtype; }
  std::string GetRunType() const { return m_run_type; }

  std::vector<std::string> GetFiles(const unsigned int flx_number);

 private:

  std::string m_name{"mvtx_file_finder"};
  std::string m_data_path{"/sphenix/lustre01/sphnxpro/commissioning/MVTX"};
  
  
  int m_run_number{0};
  std::string m_run_number_str{"0"};

  std::string m_run_type{"void"};
  std::vector<std::string> m_valid_runtypes {};

  std::map<unsigned int, std::vector<std::string>> m_file_list {};

  void CheckDataDir();
  void CheckRunType();
  void FormatRunNumber();
  void FindFiles();


};

#endif
