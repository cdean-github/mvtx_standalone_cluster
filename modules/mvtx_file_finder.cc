
#include "mvtx_file_finder.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem> // For directory operations
#include <stdexcept> // For exceptions
#include <iomanip>
#include <cstdio> // For popen
#include <regex> // For regex matching


void mvtx_file_finder::CheckDataDir() {

    // Check if the data path exists
    if (!std::filesystem::exists(m_data_path)) {
        throw std::runtime_error("Data directory does not exist: " + m_data_path);
    }

    // Check if it's a directory
    if (!std::filesystem::is_directory(m_data_path)) {
        throw std::runtime_error("Data path is not a directory: " + m_data_path);
    }

    // Clear the existing run types
    m_valid_runtypes.clear();

    // Iterate over directories in the data path
    for (const auto& entry : std::filesystem::directory_iterator(m_data_path)) {
        // Check if it's a directory
        if (entry.is_directory()) {
            // Get the directory name (run type)
            std::string run_type = entry.path().filename().string();
            // Store the run type
            m_valid_runtypes.push_back(run_type);
        }
    }

    return;
}

void mvtx_file_finder::FormatRunNumber() {
    // Convert the integer run number to a string
    std::stringstream ss;
    ss << std::setw(8) << std::setfill('0') << m_run_number;
    m_run_number_str = ss.str();
    return;
}

void mvtx_file_finder::FindFiles() {
    // Clear the existing file list
    m_file_list.clear();

    // Loop through all flxnumbers (0 to 5)
    for (unsigned int flx_number = 0; flx_number < 6; ++flx_number) {
        // Formulate the bash command with the wildcard pattern
        std::string bash_command = "ls " + m_data_path + "/" + m_run_type + "/*mvtx" + std::to_string(flx_number) + "*" + m_run_number_str + "*.evt";

        // Open a pipe to the command
        FILE* pipe = popen(bash_command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Error executing command: " << bash_command << std::endl;
            continue; // Move to the next flx_number
        }

        // Read the output of the command
        char buffer[4096];
        std::vector<std::string> files;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            // Remove newline characters from the buffer
            std::string file_name(buffer);
            file_name.erase(std::remove(file_name.begin(), file_name.end(), '\n'), file_name.end());
            files.push_back(file_name);
        }

        // Close the pipe
        pclose(pipe);

        // Store the files found for this flx_number
        if (files.empty())
        {
        //   std::cerr << "Could Not find files for" << m_run_type << " " << m_run_number_str << " flx " << flx_number << std::endl;
            //try .prdf
            bash_command = "ls " + m_data_path + "/" + m_run_type + "/*mvtx" + std::to_string(flx_number) + "*" + m_run_number_str + "*.prdf";
            pipe = popen(bash_command.c_str(), "r");
            if (!pipe) {
                std::cerr << "Error executing command: " << bash_command << std::endl;
                continue; // Move to the next flx_number
            }

            // Read the output of the command
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                // Remove newline characters from the buffer
                std::string file_name(buffer);
                file_name.erase(std::remove(file_name.begin(), file_name.end(), '\n'), file_name.end());
                files.push_back(file_name);
            }

            // Close the pipe
            pclose(pipe);

            if (files.empty())
            {
                std::cerr << "Could Not find files for" << m_run_type << " " << m_run_number_str << " flx " << flx_number << std::endl;
                std::cerr << "Tried both .evt and .prdf" << std::endl;
                std::cerr << "Skipping flx " << flx_number << std::endl;
                continue;
            }
        }

        // add flx_number to map
        if (m_file_list.find(flx_number) != m_file_list.end())
        {
          std::cerr << "Duplicate flx_number found in file list" << std::endl;
        }

        m_file_list[flx_number] = files;
    }
}

void mvtx_file_finder::CheckRunType() {

    bool isvalid = false;
    // Check if the run type is in the m_valid_runtypes vector
    for (const auto& valid_runtype : m_valid_runtypes) {
        if (m_run_type == valid_runtype) {
            isvalid = true;
            break;
        }
    }
    if(!isvalid)
    {
      std::cerr << "Invalid run type: " << m_run_type << std::endl;
      std::cerr << "Valid Run types: ";
      for (const auto& valid_runtype : m_valid_runtypes) {
      std::cerr << valid_runtype << " ";
      }

      std::cerr << std::endl;

      throw std::runtime_error("Invalid run type: " + m_run_type);

    }
}

std::vector<std::string> mvtx_file_finder::GetFiles(const unsigned int flx_number) {
    // Check if the directory has been checked and files filled
    if (flx_number > 5 || flx_number < 0) {
        throw std::runtime_error("Invalid flx_number: " + std::to_string(flx_number));
    }

    if (m_file_list.empty()) {
        // Directory not checked, call CheckDataDir and FormatRunNumber
        CheckDataDir();
        CheckRunType();
        FormatRunNumber();
        // Now fill the file list
        FindFiles();
    }

    // Return the requested files for the given flx_number
    return m_file_list[flx_number];
}



