#pragma once

//Includes
#include <cstdint>
#include <functional>
#include <Windows.h>
#include <Tlhelp32.h>

namespace processtweaker
{

  static void do_process(std::function<bool(PROCESSENTRY32&)> callback)
  {
    //Create tool help snapshot
    HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    PROCESSENTRY32 proc_entry{ 0 };
    proc_entry.dwSize = sizeof proc_entry;
    
    //Get first process entry
    Process32First(h_snapshot, &proc_entry);
    
    do
    {
      //Run callback if callback returns true exit loop.
      if(callback(proc_entry))
        break;    
    }
    //Get next process entry
    while(Process32Next(h_snapshot, &proc_entry));
        
    //Close handle to snapshot
    CloseHandle(h_snapshot);
  }

  //Loop through process and get ID of requested process
  static std::uint32_t get_proc_id(const wchar_t* process_name)
  {
    std::uint32_t proc_id = 0;
    
    //Iterate through each process
    do_process([&](PROCESSENTRY32& proc_entry) -> bool 
    {
      
      //Check if process names match
      if(!_wcsicmp(proc_entry.szExeFile, process_name))
      {
        //Set return value and exit loop
        proc_id = proc_entry.th32ProcessID;
        return true;
      }
      
    });
    
    return proc_id; 
  }

  class process
  {
  private:
    HANDLE h_proc = nullptr;
    std::uint32_t proc_id = 0;
    
  public:
    //Example name GTA5.exe
    process(const wchar_t* process_name, std::uint32_t access = PROCESS_ALL_ACCESS)
    {
      proc_id = get_proc_id(process_name);
      h_proc = OpenProcess(access, false, proc_id);
    }
    
    template<typename T>
    T& read(std::uintptr_t address)
    {
      size_t buffer_size = sizeof T;
      T buffer;
      
      if(!ReadProcessMemory(h_proc, (LPVOID)address, &buffer, buffer_size, nullptr))
        printf_s("[processtweaker] Failed to read process memory at %p\n", address);
        
      return buffer;
    }
    
    template<typename T>
    void write(std::uintptr_t address, T& buffer)
    {
      size_t buffer_size = sizeof T;

      if(!WriteProcessMemory(h_proc, (LPVOID)address, &buffer, buffer_size, nullptr))
        printf_s("[processtweaker] Failed to write process memory at %p\n", address);
    }
    
    void do_module(std::functional<bool(MODULEENTRY32&) callback)
    {
      //Create tool help snapshot
      HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 0);
    
      MODULEENTRY32 module_entry{ 0 };
      module_entry.dwSize = sizeof module_entry;
    
      //Get first module entry
      Module32First(h_snapshot, &module_entry);
    
      do
      {
        //Run callback. if callback returns true exit loop.
        if(callback(module_entry))
          break;    
      }
      //Get next module entry
      while(Module32Next(h_snapshot, &module_entry));
        
      //Close handle to snapshot
      CloseHandle(h_snapshot);
    }
    
    std::uintptr_t get_module_base(const wchar_t* module_name)
    {
      std::uintptr_t module_base = 0;
      
      do_module([&](MODULEENTRY32& module_entry) -> bool
      {
        
        if(_wcsicmp(module_name, module_entry.szModule))
        {
          module_base = (std::uintptr_t)module_entry.modBaseAddr;
          return true;
        }
        
        return false
      }
    
      return module_base;    
    }
    
    
  }
}
