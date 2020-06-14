#include <xcom/workspace.hpp>

namespace cx::workspace
{
    Workspace::Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space) :
        m_id(ws_id), m_name(ws_name), 
        m_space(space), 
        m_containers(nullptr), 
        m_floating_containers{},
        m_root{std::make_unique<ContainerTree>("root container", geom::Geometry::default_new())},
        focused_container(nullptr), 
        is_pristine(true)
    {
        focused_container = m_root.get();
    }

    auto Workspace::register_window(Window window, bool tiled) -> std::optional<SplitConfigurations> {
        DBGLOG("Warning - function {} not implemented or implemented to test simple use cases", "Workspace::register_window");
        if(tiled) { 
            if(!focused_container->is_window()) {
                focused_container->push_client(window);
                return SplitConfigurations{focused_container->client.value()};
            } 
            else {
                focused_container->push_client(window);
                auto existing_win = focused_container->left->client.value();
                auto new_win = focused_container->right->client.value();
                focused_container = focused_container->right.get();
                return SplitConfigurations{existing_win, new_win};
            }
            // are we "dirty"? If so, re-map the workspace 
            is_pristine = false;
        } else {
            m_floating_containers.push_back(std::move(window));
            return {};
        }
    }

    std::unique_ptr<Window> Workspace::unregister_window(Window* w) {
        cx::println("Unregistering windows not yet implemented");
        std::abort();
    }
};
