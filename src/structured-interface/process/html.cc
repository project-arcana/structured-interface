#include "html.hh"

#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <structured-interface/gui.hh>
#include <structured-interface/properties.hh>
#include <structured-interface/recorded_ui.hh>

cc::string si::process_html(gui& ui, recorded_ui const& record)
{
    cc::string html;

    html += "<!doctype html>\n";
    html += "<html lang=\"en\">\n";
    html += "<body>\n";

    struct visitor
    {
        cc::string& html;

        cc::vector<cc::string> tags;

        void start_element(size_t id, element_type type)
        {
            auto start_tag = [&](cc::string t) {
                if (t != "")
                    html += "<" + t + ">\n";
                tags.push_back(t);
            };

            switch (type)
            {
            case element_type::text:
                start_tag("p");
                break;
            case element_type::button:
                start_tag("button");
                break;
            default:
                tags.push_back("");
                break;
            }
        }
        void property(size_t prop_id, cc::span<cc::byte const> value)
        {
            if (prop_id == si::property::text.id())
            {
                html += cc::string_view((char const*)value.data(), value.size());
                html += "\n";
            }
        }
        void end_element()
        {
            auto t = tags.back();
            tags.pop_back();
            if (t != "")
                html += "</" + t + ">\n";
        }
    };

    auto v = visitor{html};
    record.visit(v);

    html += "</body>\n";
    html += "</html>\n";
    return html;
}
