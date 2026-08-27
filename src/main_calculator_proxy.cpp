/**
 * calculator_client.cpp
 *
 * Lightweight FTXUI frontend for calculator_proxy_t.
 * Layout: left panel = controls/inputs, right panel = response log.
 *
 * Build (adjust include/lib paths to your tree):
 *   g++ -std=c++20 calculator_client.cpp \
 *       -I<threesomeip_include> -I<ftxui_include> \
 *       -L<threesomeip_lib> -lthreesomeip \
 *       -L<ftxui_lib> -lftxui-component -lftxui-dom -lftxui-screen \
 *       -lpthread -o calculator_client
 */

/*=====*\
 * C++ *
\*=====*/
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/*=========*\
 * FTXUI   *
\*=========*/
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

/*===============*\
 * APPLICATION   *
\*===============*/
#include <runtime_proxy.hpp>
#include <calculator_proxy.hpp>
#include <configuration.hpp>
#include <active_object.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>


using namespace ftxui;
using namespace threesomeip;


static std::condition_variable g_cv;
static std::mutex              g_mu;
static bool                    g_stop = false;


static void signal_handler(int) {
    std::lock_guard lk(g_mu);
    g_stop = true;
    g_cv.notify_all();
}

// ---------------------------------------------------------------------------
// Log helper (thread-safe, wakes UI)
// ---------------------------------------------------------------------------
struct Log {
    std::vector<std::string> lines;
    std::mutex               mu;
    ScreenInteractive*       screen = nullptr;

    void push(std::string msg) {
        {
            std::lock_guard lk(mu);
            lines.push_back(std::move(msg));
        }
        if (screen)
            screen->PostEvent(Event::Custom); // wake render loop
    }

    Elements render() {
        std::lock_guard lk(mu);
        Elements elems;
        elems.reserve(lines.size());
        for (auto& l : lines)
            elems.push_back(text(l));
        return elems;
    }
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    const std::string app_name{"calculator_proxy"};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S.%e][" + app_name + "][%n]%^[%l] %v%$");
    console_sink->set_color(spdlog::level::info, console_sink->green);
    console_sink->set_color(spdlog::level::warn, console_sink->yellow);
    console_sink->set_color(spdlog::level::err, console_sink->red);
    console_sink->set_color(spdlog::level::critical, console_sink->red_bold);

    auto main_logger = std::make_shared<spdlog::logger>(app_name, console_sink);
    spdlog::register_logger(main_logger);

    auto active_object = utils::active_object_factory::make_active_object(app_name);

    // --- runtime + proxy setup -------------------------------------------
    threesomeip::runtime::runtime_proxy_t runtime_proxy{
        active_object,
        "/home/adjurdjevic/Desktop/threesomeip/ipc_sockets",
        "calculator_proxy",
        uint16_t{1},
        "runtime",
        {},
        std::vector<config::service_configuration_t>{
            config::service_configuration_t{
                uint16_t{0x1234},
                uint16_t{0x0000},
                uint16_t{31000},
                uint16_t{30506}
            }
        }
    };

    calculator::calculator_proxy_t        proxy(runtime_proxy);


    Log log;

    // --- input state for add() -------------------------------------------
    std::string input_a, input_b;
    std::string input_precision;

    // --- build UI ----------------------------------------------------------
    auto screen = ScreenInteractive::Fullscreen();
    log.screen  = &screen;

    // Input fields
    auto input_a_comp         = Input(&input_a,         "a");
    auto input_b_comp         = Input(&input_b,         "b");
    auto input_precision_comp = Input(&input_precision, "uint32");

    // Buttons
    auto btn_add = Button("Add(a, b)", [&] {
        try {
            float a = std::stof(input_a);
            float b = std::stof(input_b);
            auto  f = proxy.add(a, b);
            // Resolve on a detached thread so UI stays responsive
            std::thread([&log, f = std::move(f), a, b]() mutable {
                float result = f.get();
                log.push("add(" + std::to_string(a) + ", " +
                         std::to_string(b) + ") -> " +
                         std::to_string(result));
            }).detach();
        } catch (...) {
            log.push("[add] invalid input — enter two floats");
        }
    });

    auto btn_beepboop = Button("BeepBoop (F&F)", [&] {
        proxy.beepboop();
        log.push("beepboop() fired");
    });

    auto btn_get_precision = Button("Get Precision", [&] {
        auto f = proxy.get_precision();
        std::thread([&log, f = std::move(f)]() mutable {
            uint32_t val = f.get();
            log.push("get_precision() -> " + std::to_string(val));
        }).detach();
    });

    auto btn_set_precision = Button("Set Precision", [&] {
        try {
            uint32_t val = static_cast<uint32_t>(std::stoul(input_precision));
            auto     f   = proxy.set_precision(val);
            std::thread([&log, f = std::move(f), val]() mutable {
                f.get();
                log.push("set_precision(" + std::to_string(val) + ") -> ok");
            }).detach();
        } catch (...) {
            log.push("[set_precision] invalid input — enter a uint32");
        }
    });

    auto btn_clear = Button("Clear Log", [&] {
        std::lock_guard lk(log.mu);
        log.lines.clear();
    });

    auto btn_quit = Button("Quit", screen.ExitLoopClosure());

    // Component tree
    auto controls = Container::Vertical({
        // add()
        Container::Horizontal({ input_a_comp, input_b_comp }),
        btn_add,
        // beepboop()
        btn_beepboop,
        // get_precision()
        btn_get_precision,
        // set_precision()
        input_precision_comp,
        btn_set_precision,
        // utility
        btn_clear,
        btn_quit,
    });

    auto renderer = Renderer(controls, [&] {
        return hbox({
            // ---- left panel: controls ----
            vbox({
                text("calculator_proxy_t") | bold | hcenter,
                separator(),

                text("add(float a, float b)") | dim,
                hbox({
                    input_a_comp->Render()         | border | size(WIDTH, EQUAL, 12),
                    text(" + "),
                    input_b_comp->Render()         | border | size(WIDTH, EQUAL, 12),
                }),
                btn_add->Render()                  | hcenter,
                separator(),

                text("beepboop()") | dim,
                btn_beepboop->Render()             | hcenter,
                separator(),

                text("get_precision()") | dim,
                btn_get_precision->Render()        | hcenter,
                separator(),

                text("set_precision(uint32_t)") | dim,
                input_precision_comp->Render()     | border,
                btn_set_precision->Render()        | hcenter,
                separator(),

                filler(),
                btn_clear->Render()                | hcenter,
                btn_quit->Render()                 | hcenter,
            }) | border | size(WIDTH, EQUAL, 40),

            // ---- right panel: response log ----
            vbox({
                text("Responses") | bold | hcenter,
                separator(),
                vbox(log.render()) | flex,
            }) | border | flex,
        });
    });

    // --- event loop (parks main thread) -----------------------------------
    // Worker thread: watches SIGINT/SIGTERM and triggers UI exit
    std::thread signal_watcher([&] {
        std::unique_lock lk(g_mu);
        g_cv.wait(lk, [] { return g_stop; });
        screen.ExitLoopClosure()();
    });

    screen.Loop(renderer); // blocks until ExitLoopClosure is called

    // --- teardown ----------------------------------------------------------
    {
        std::lock_guard lk(g_mu);
        g_stop = true;
        g_cv.notify_all();
    }
    signal_watcher.join();

    return 0;
}