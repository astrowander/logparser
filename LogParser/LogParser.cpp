#include <cstdint>
#include <fstream>
#include <exception>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

static bool __forceinline IsDomainChar(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch == '.') || (ch == '-');
}

static bool __forceinline IsPathChar(char ch)
{
    return (ch == '/') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch == '.') || (ch == ',') || (ch == '+') || (ch == '_');
}

class StreamBuffer
{
    static const uint32_t _bufferSize = 64*1024;

    char _buffer[_bufferSize];
    char * _pChar = nullptr;

    uint32_t _charsLeft = 0;
    std::ifstream _input;

    bool _eof = false;

public:

    void Open(const std::string & filename)
    {
        _input.open(filename);
    }

    void Close()
    {
        _input.close();
    }

    char __forceinline GetChar()
    {
        if (_charsLeft != 0)
        {
            --_charsLeft;
            return *_pChar++;
        }

        if (!_input.eof())
        {
            _input.read(_buffer, _bufferSize);
            _pChar = _buffer;
            _charsLeft = (uint32_t)_input.gcount() - 1;
            return *_pChar++;
        }

        _eof = true;
        return 0;
    }

    bool Eof()
    {
        return _eof;
    }
};

class UrlStatistics
{
    uint32_t _N = 1;
    uint32_t _urlsCount = 0;

    std::unordered_map<std::string, uint32_t> _domains;
    std::unordered_map<std::string, uint32_t> _paths;

    std::ofstream _output;

    StreamBuffer _streamBuffer;

public:
    UrlStatistics(std::vector<std::string> const & arguments)
    {
       if (arguments[1] == "-n")
       {
           if (arguments.size() < 5)
               throw std::invalid_argument("Too few arguments");

            _N = std::stoi(arguments[2]);

            _streamBuffer.Open(arguments[3]);
            _output.open(arguments[4], std::ios_base::binary | std::ios_base::out);
       }
       else
       {
           if (arguments.size() < 3)
               throw std::invalid_argument("Too few arguments");

           _streamBuffer.Open(arguments[1]);
           _output.open(arguments[2], std::ios_base::binary | std::ios_base::out);
       }
    }

    void GenerateStatistics()
    {
        while (!_streamBuffer.Eof())
        {
            char ch = _streamBuffer.GetChar();
            if (ch != 'h'/* && ch != 'H'*/) continue;

            ch = _streamBuffer.GetChar();
            if (ch != 't' /*&& ch != 'T'*/) continue;

            ch = _streamBuffer.GetChar();
            if (ch != 't' /*&& ch != 'T'*/) continue;

            ch = _streamBuffer.GetChar();
            if (ch != 'p' /*&& ch != 'P'*/) continue;

            ch = _streamBuffer.GetChar();
            if (ch == 's'  /*&& ch != 'S'*/) ch = _streamBuffer.GetChar();
            if (ch != ':')  continue;

            if (_streamBuffer.GetChar() != '/') continue;
            if (_streamBuffer.GetChar() != '/') continue;

            std::string domain;
			domain.reserve(256);
            ch = _streamBuffer.GetChar();

            while (IsDomainChar(ch))
            {
                domain.push_back(ch);
                ch = _streamBuffer.GetChar();
            }

            if (domain.empty()) continue;

            ++_urlsCount;

            auto it = _domains.find(domain);
            if (it == _domains.end())
            {
                _domains.insert(std::make_pair(domain, 1));
            }
            else
            {
                ++it->second;
            }

            std::string path;
			path.reserve(256);

            if (ch == '/')
            while (IsPathChar(ch))
            {
                path.push_back(ch);
                ch = _streamBuffer.GetChar();
            }

            if (path.empty())
                path.push_back('/');

            it = _paths.find(path);
            if (it == _paths.end())
            {
                _paths.insert(std::make_pair(path, 1));
            }
            else
            {
                ++it->second;
            }
        }
    }

	void OutputStatistics()
    {
		_output << "total urls " << _urlsCount << ", domains " << _domains.size() << ", paths " << _paths.size() << "\n\n";
		
        std::multimap<uint32_t, std::string> topDomains;
        for (auto it = _domains.begin(); it != _domains.end(); ++it)
        {
            topDomains.insert(std::make_pair(it->second, it->first));
        }

        _output << "top domains" << "\n";
        uint32_t i = 0;
        auto rit = topDomains.rbegin();

        while (i < _N && rit != topDomains.rend())
        {
            _output << rit->first << " " << rit->second << "\n";
            ++i;
            ++rit;
        }

        _output << "\n";

        std::multimap<uint32_t, std::string> topPaths;
        for (auto it = _paths.begin(); it != _paths.end(); ++it)
        {
            topPaths.insert(std::pair<uint32_t, std::string>(it->second, it->first));
        }

        i = 0;
        rit = topPaths.rbegin();

        _output << "top paths" << "\n";
        while (i < _N && rit != topPaths.rend())
        {
            _output << rit->first << " " << rit->second << "\n";
            ++i;
            ++rit;
        }
    }

    ~UrlStatistics()
    {
        _streamBuffer.Close();
        _output.close();
    }
};

int main(int argc, char *argv[])
{
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    try
    {
        UrlStatistics statistics(arguments);
		statistics.GenerateStatistics();
		statistics.OutputStatistics();		
    }
    catch (std::exception & e)
    {
        std::cout << "Error:" << e.what() << "\n";
    }
}


