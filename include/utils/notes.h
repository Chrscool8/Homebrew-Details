#include <string>

void read_notes();
void save_notes();
void notes_set_value(std::string key, std::string value);
std::string notes_get_value(std::string key);

extern nlohmann::json notes_json;
