#ifndef YAVE_PIPELINE_NODES_H
#define YAVE_PIPELINE_NODES_H

#include <yave/yave.h>
#include <yave/swapchain/FrameToken.h>

namespace yave {

class RenderingNodeProcessor;
class RenderingNode;

class NodeBase : NonCopyable {
	public:
		virtual ~NodeBase() {
		}

	protected:
		friend class RenderingNode;

		virtual void compute_dependencies(const FrameToken&, RenderingNode&) = 0;
};



class Node : public NodeBase {
	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&) = 0;
};


class SecondaryRenderer : public DeviceLinked, public NodeBase {
	public:
		SecondaryRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<CmdBufferUsage::Secondary>&&) = 0;
};


class Renderer : public DeviceLinked, public NodeBase {
	public:
		Renderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual TextureView view() const = 0;

		math::Vec2ui size() const {
			return view().size();
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

class EndOfPipeline : public DeviceLinked, public NodeBase {
	public:
		EndOfPipeline(DevicePtr dptr) : DeviceLinked(dptr) {
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

}

#endif // YAVE_PIPELINE_NODES_H
