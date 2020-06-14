#include <xcom/container.hpp>
#ifdef DEBUGGING
#include <cassert>
#endif

namespace cx::workspace
{
    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry) :   
        tag(std::move(container_tag)), client{}, 
        left(nullptr), right(nullptr), parent(nullptr), 
        policy(Layout::Vertical), split_ratio(0.5),
        geometry(geometry)
    {
        assert(!client.has_value() && "Client should not be set using this constructor");
    }

    ContainerTree::~ContainerTree() { }
    
    bool ContainerTree::is_split_container() const {
        return !client.has_value();
    }

    bool ContainerTree::is_window() const {
        return client.has_value();
    }
    // TODO(fix): This assumes we are always focusing a "bottom-most" window. So this will
    // cause errors in display, if we focus on a window that isn't newly created. This will of course be fixed.
    // I just want to see that mapping 4 windows of equal size to each corner works initially & automatically
    void ContainerTree::push_client(Window new_client) {
        if(is_window()) {
            DBGLOG("Container {} is window", tag);
            auto existing_client = client.value();
            this->tag = "split_container";
            if(policy == Layout::Horizontal) {
                auto container_geometry = geom::vsplit(this->geometry);
                auto& [left_geo, right_geo] = container_geometry;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, left_geo);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, right_geo);
                left->push_client(existing_client);
                right->push_client(new_client);
                client.reset();
            } else if(policy == Layout::Vertical) {
                auto container_geometry = geom::hsplit(this->geometry);
                auto& [top, bottom] = container_geometry;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, top);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, bottom);
                left->push_client(existing_client);
                right->push_client(new_client);
                client.reset();
            }
        } else {
            DBGLOG("Container {} is not window", tag);
            new_client.set_geometry(this->geometry);
            this->tag = new_client.m_tag.m_tag;
            this->client = new_client;
            this->client.value().set_geometry(this->geometry);
        }
    }

} // namespace cx::workspace
