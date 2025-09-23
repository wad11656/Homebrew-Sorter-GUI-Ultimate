#pragma once
#include <string>
#include <vector>
#include <stdint.h>

// ---------- Titles ----------
bool readIsoTitle(const std::string& path, std::string& outTitle);
bool readCompressedIsoTitle(const std::string& path, std::string& outTitle); // .cso / .zso
bool readJsoTitle(const std::string& path, std::string& outTitle);           // .jso
bool readDaxTitle(const std::string& path, std::string& outTitle);           // .dax

// ---------- ICON0.PNG bytes ----------
bool readJsoIconPNG(const std::string& path, std::vector<uint8_t>& outVec);  // .jso
bool readDaxIconPNG(const std::string& path, std::vector<uint8_t>& outVec);  // .dax

// Convenience: choose ISO/CSO/ZSO/DAX/JSO automatically; returns PNG bytes
bool ExtractIcon0PNG(const std::string& path, std::vector<uint8_t>& outVec);

// Optional tiny link-probe (used by your app)
extern "C" int cmfe_titles_extras_present();
