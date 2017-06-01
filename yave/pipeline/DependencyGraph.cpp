#include "DependencyGraph.h"

#include <unordered_set>

namespace yave {


namespace detail {

NodeData::NodeData(NotOwner<Node*> node) : _type(Type::Node), _node(node) {
}

NodeData::NodeData(NotOwner<SecondaryRenderer*> node, const Framebuffer& framebuffer) : _type(Type::Secondary), _node(node), _framebuffer(&framebuffer) {
}

NodeData::NodeData(NotOwner<Renderer*> node) : _type(Type::Primary), _node(node) {
}

void NodeData::process(DevicePtr dptr, const FrameToken& token, CmdBufferRecorder<>& recorder) {
	switch(_type) {
		case Type::Node:
			dynamic_cast<Node*>(_node)->process(token);
		break;

		case Type::Secondary:
			dynamic_cast<SecondaryRenderer*>(_node)->process(token, CmdBufferRecorder<CmdBufferUsage::Secondary>(dptr->create_secondary_cmd_buffer(), *_framebuffer));
		break;

		case Type::Primary:
			dynamic_cast<Renderer*>(_node)->process(token, recorder);
		break;

		default:
			fatal("Unknown dependency type.");
	}
}


bool NodeData::operator==(const NodeData& other) const {
	return _node == other._node && _framebuffer == other._framebuffer;
}

bool NodeData::operator!=(const NodeData& other) const {
	return !operator==(other);
}

usize NodeData::Hash::operator()(const NodeData& data) const {
	return hash<std::initializer_list<const void*>>({reinterpret_cast<const void*>(data._framebuffer), reinterpret_cast<const void*>(data._node)});
}

}




DependencyGraph::DependencyGraph(Renderer* renderer) : _renderer(renderer) {
	DependencyGraphNode(renderer, *this);
}

RecordedCmdBuffer<> DependencyGraph::build_command_buffer(DevicePtr dptr, const FrameToken& token) {
	using namespace detail;

	CmdBufferRecorder<> recorder = dptr->create_cmd_buffer();

	core::Vector<NodeData> ready = {NodeData(_renderer)};
	core::Vector<NodeData> done;
	auto left = _dependencies;

	while(done.size() != _dependencies.size()) {
		std::for_each(ready.begin(), ready.end(), [&](auto& d) { d.process(dptr, token, recorder); });
		std::copy(ready.begin(), ready.end(), std::back_inserter(done));
		ready.clear();

		for(const auto& p : left) {
			if(std::all_of(p.second.begin(), p.second.end(), [&](const NodeData& d) {
					return std::find(done.begin(), done.end(), d) != done.end();
				})) {

				ready.push_back(p.first);
			}
		}

		for(const auto& node : ready) {
			left.erase(left.find(node));
		}
	}

	return recorder.end();
}



DependencyGraphNode::DependencyGraphNode(const NodeData& node, DependencyGraph& graph) : _dependencies(graph._dependencies[node]), _graph(graph) {
	node._node->compute_dependencies(*this);
}

void DependencyGraphNode::add_node_dependency(const NodeData& data) {
	_dependencies.push_back(data);
	if(_graph._dependencies.find(data) == _graph._dependencies.end()) {
		DependencyGraphNode(data, _graph);
	}
}


}
