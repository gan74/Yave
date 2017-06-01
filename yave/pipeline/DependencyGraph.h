#ifndef YAVE_PIPELINE_DEPENDENCYGRAPH_H
#define YAVE_PIPELINE_DEPENDENCYGRAPH_H

#include "pipeline.h"

namespace yave {

namespace detail {

struct NodeData {
	enum class Type {
		Node,
		Secondary,
		Primary
	};

	NodeData(NotOwner<Node*> node);
	NodeData(NotOwner<SecondaryRenderer*> node, const Framebuffer& framebuffer);
	NodeData(NotOwner<Renderer*> node);

	void process(DevicePtr dptr, const FrameToken& token, CmdBufferRecorder<>& recorder);


	Type _type;
	NodeBase* _node = nullptr;
	const Framebuffer* _framebuffer = nullptr;

	bool operator==(const NodeData& other) const;
	bool operator!=(const NodeData& other) const;

	struct Hash {
		usize operator()(const NodeData& data) const;
	};
};

}

class DependencyGraph : NonCopyable {
	using NodeData = detail::NodeData;

	public:
		DependencyGraph(Renderer* renderer);

		RecordedCmdBuffer<> build_command_buffer(DevicePtr dptr, const FrameToken& token);

	public:
		Renderer* _renderer;
		std::unordered_map<NodeData, core::Vector<NodeData>, NodeData::Hash> _dependencies;
};

class DependencyGraphNode : NonCopyable {
	using NodeData = detail::NodeData;

	public:
		template<typename... Args>
		void add_dependency(Args&&... args) {
			add_node_dependency(NodeData(std::forward<Args>(args)...));
		}


	private:
		friend class DependencyGraph;

		DependencyGraphNode(const NodeData& node, DependencyGraph& graph);

	private:
		void add_node_dependency(const NodeData& data);

		core::Vector<NodeData>& _dependencies;
		DependencyGraph& _graph;
};

}

#endif // YAVE_PIPELINE_DEPENDENCYGRAPH_H
