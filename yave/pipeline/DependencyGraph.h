#ifndef YAVE_PIPELINE_DEPENDENCYGRAPH_H
#define YAVE_PIPELINE_DEPENDENCYGRAPH_H

#include "nodes.h"

namespace yave {

namespace detail {

struct NodeData {
	enum class Type {
		Node,
		Secondary,
		Primary,
		End
	};

	NodeData(const FrameToken& token, NotOwner<Node*> node);
	NodeData(const FrameToken& token, NotOwner<SecondaryRenderer*> node, const Framebuffer& framebuffer);
	NodeData(const FrameToken& token, NotOwner<Renderer*> node);
	NodeData(const FrameToken& token, NotOwner<EndOfPipeline*> node);

	void process(DevicePtr dptr, CmdBufferRecorder<>& recorder);


	Type _type;
	FrameToken _token;

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
		DependencyGraph(const FrameToken& token, EndOfPipeline* renderer);

		RecordedCmdBuffer<> build_command_buffer(DevicePtr dptr);

	public:
		NodeData _root;
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
