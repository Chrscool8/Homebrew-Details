#include <sys/select.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>
//
#include <curl/curl.h>
#include <curl/easy.h>

s_online_info online_info;

void init_online_info()
{
    online_info.online_version_name        = "None";
    online_info.online_version_number      = "0";
    online_info.online_version_description = "No Desc.";
    online_info.update_available           = false;
    online_info.online_version_date        = "x";
}

std::string get_online_version_name()
{
    return online_info.online_version_name;
}

std::string get_online_version_number()
{
    return online_info.online_version_number;
}

std::string get_online_version_description()
{
    return online_info.online_version_description;
}

std::string get_online_version_date()
{
    return online_info.online_version_date;
}

bool get_online_version_available()
{
    return online_info.update_available;
}

void set_online_version_name(std::string name)
{
    online_info.online_version_name = name;
}

void set_online_version_number(std::string number)
{
    online_info.online_version_number = number;
}

void set_online_version_description(std::string desc)
{
    online_info.online_version_description = desc;
}

void set_online_version_available(bool available)
{
    online_info.update_available = available;
}

void set_online_version_date(std::string date)
{
    online_info.online_version_date = date;
}

size_t CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

bool check_for_updates()
{
    print_debug("curl time\n");

    CURL* curl;
    CURLcode res;

    print_debug("curl globally initting\n");
    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (curl)
    {
        print_debug("curl easily initted\n");
        std::string s;

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/Chrscool8/Homebrew-Details/releases/latest");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Homebrew-Details");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

        res = curl_easy_perform(curl);
        print_debug("curl easily performed\n");
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        if (res == CURLE_OK)
        {
            nlohmann::json j = nlohmann::json::parse(s);

            set_online_version_number(parse_version(json_load_value_string(j, "tag_name")));
            set_online_version_name(json_load_value_string(j, "name"));
            set_online_version_description(json_load_value_string(j, "body"));
            set_online_version_date(json_load_value_string(j, "created_at"));

            print_debug((std::string("") + get_online_version_number() + " : " + get_setting(setting_local_version) + "\n").c_str());
            if (is_number(get_online_version_number()) && is_number(get_setting(setting_local_version)))
            {
                print_debug("nums\n");
                if (std::stod(get_online_version_number()) > std::stod(get_setting(setting_local_version)))
                {
                    print_debug("need up\n");
                    set_online_version_available(true);
                    return true;
                }
            }
        }
        else
        {
            print_debug(std::string("Curl Error: ") + curl_easy_strerror(res));
        }
    }

    if (get_setting_true(setting_debug))
    {
        print_debug("debug force up\n");
        set_online_version_available(true);
        return true;
    }

    print_debug("no update\n");
    set_online_version_available(false);
    return false;
}