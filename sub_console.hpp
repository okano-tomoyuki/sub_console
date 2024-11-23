#ifndef SUB_CONSOLE_HPP
#define SUB_CONSOLE_HPP

#if !defined(_WIN32) && !defined(_WIN64)
#error SubConsole is only support windows platform now.
#endif

#include <string>
#include <sstream>
#include <stdexcept>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif

class SubConsole
{

public:
    std::string error_msg;
    
    SubConsole(const std::string& pipe_name = "", const int& buffer_size = 1024)
    {
        if (!pipe_name.empty())
            open(pipe_name, buffer_size);
    }

    ~SubConsole()
    {
        close();
    }

    bool open(const std::string& pipe_name, const int& buffer_size)
    {
        pipe_handle_ = CreateNamedPipeA(std::string("\\\\.\\pipe\\" + pipe_name).c_str() , PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,	2, 0, 0, 10, NULL);
        
        if (pipe_handle_ == NULL || pipe_handle_ == INVALID_HANDLE_VALUE)
        {
            std::stringstream ss;
            ss << __func__ << "failed in 'CreateNamedPipeA' function. code :" << GetLastError() << std::endl;
            throw_error(ss.str());
            return false;
        }

        PROCESS_INFORMATION pi;
        STARTUPINFOA si = {sizeof(STARTUPINFOA)};
        if (!CreateProcessA(NULL, const_cast<char *>(ps1_script(pipe_name, buffer_size).c_str()), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            CloseHandle(pipe_handle_);
            std::stringstream ss;
            ss << __func__ << "failed in 'CreateProcessA' function. code :" << GetLastError() << std::endl;
            throw_error(ss.str());
            return false;
        }
        CloseHandle(pi.hThread);

        if (!ConnectNamedPipe(pipe_handle_, NULL))
        {
            CloseHandle(pipe_handle_);
            CloseHandle(pi.hProcess);
            std::stringstream ss;
            ss << __func__ << "failed in 'ConnectNamedPipe' function. code :" << GetLastError() << std::endl;
            throw_error(ss.str());
            return false;
        }
        process_handle_ = pi.hProcess;
        return true;
    }

    bool write(const std::string& msg)
    {
        DWORD total_size = 0;
        DWORD written_size;
        while(WriteFile(pipe_handle_, msg.c_str(), msg.size(), &written_size, NULL))
        {
            total_size += written_size;
            if (total_size == msg.size())
                return true;
        }
        return false;
    }

    bool clear()
    {
        return write("\e[2J\e[1;1H");
    }

    void close()
    {
        write("@@@@@@@@@@@");
	    
        if (pipe_handle_ != NULL && pipe_handle_ != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(pipe_handle_);
            DisconnectNamedPipe(pipe_handle_);
            CloseHandle(pipe_handle_);
        }
        
        if (process_handle_ != NULL && process_handle_ != INVALID_HANDLE_VALUE)
        {
            WaitForSingleObject(process_handle_, INFINITE);
            CloseHandle(process_handle_);
            CloseHandle(process_handle_);
        }
    }

private:
    void* pipe_handle_     = INVALID_HANDLE_VALUE;
    void* process_handle_  = INVALID_HANDLE_VALUE;

    std::string throw_error(const std::string& msg)
    {
        error_msg += msg;
#ifndef UTILITY_NO_THROW_EXCEPTION
        throw std::runtime_error(msg);
#endif
    }

    static std::string ps1_script(const std::string& pipe_name, const int& buffer_size)
    {
        return 
        "powershell.exe -ExecutionPolicy Bypass -Command \""
        "$pipe = New-Object System.IO.Pipes.NamedPipeClientStream \".\", \"" + pipe_name + "\", InOut;"
        "$pipe.Connect();"
        "$buf  = New-Object byte[] " + std::to_string(buffer_size) + ";"
        "$loop = $true;"
        "while($loop)" 
        "{"
            "$len = $pipe.Read($buf, 0, $buf.Length);"
            "$str = [System.Text.Encoding]::ASCII.GetString($buf, 0, $len);"
            "if($str -match '@@@@@@@@@@@') { $loop = $false; } else { $str; }"
        "}"
        "$pipe.Close();\"";
    }
};

#endif // SUB_CONSOLE_HPP