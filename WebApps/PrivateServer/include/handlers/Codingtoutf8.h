#pragma once

#include <uchardet/uchardet.h>
#include <fstream>
#include <sstream>
#include <iconv.h>
#include <iostream>
#include <unordered_set>

bool isTextFile(const std::string& filename) {
    static const std::unordered_set<std::string> textExts = {
        ".txt", ".html", ".css", ".js", ".json", ".xml", ".csv", ".md"
    };
    std::string ext = boost::filesystem::extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return textExts.count(ext) > 0;
}

std::string detectEncoding(const std::string& content) {
    uchardet_t ud = uchardet_new();
    uchardet_handle_data(ud, content.c_str(), content.size());
    uchardet_data_end(ud);
    std::string encoding = uchardet_get_charset(ud);
    uchardet_delete(ud);
    return encoding;
}

std::string convertToUtf8(const std::string& input, const std::string& fromEncoding) {
    iconv_t cd = iconv_open("UTF-8", fromEncoding.c_str());
    if (cd == (iconv_t)-1) return "";

    size_t inBytesLeft = input.size();
    size_t outBytesLeft = input.size() * 4;
    std::string output(outBytesLeft, 0);

    char* inBuf = const_cast<char*>(input.c_str());
    char* outBuf = &output[0];

    if (iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft) == (size_t)-1) {
        iconv_close(cd);
        return "";
    }

    iconv_close(cd);
    output.resize(output.size() - outBytesLeft);
    return output;
}