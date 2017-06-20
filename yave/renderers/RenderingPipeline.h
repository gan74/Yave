/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_RENDERERS_RENDERINGPIPELINE_H
#define YAVE_RENDERERS_RENDERINGPIPELINE_H

#include <yave/swapchain/FrameToken.h>

#include <y/core/Functor.h>

#include <typeindex>
#include <future>
#include <any>

namespace yave {

class Node : NonCopyable {
	public:
		template<typename T>
		using Ptr = core::Arc<T>;

		virtual ~Node() {
		}
};


/*
		,     \    /      ,
	   / \    )\__/(     / \
	  /   \  (_\  /_)   /   \
 ____/_____\__\@  @/___/_____\____
|             |\../|              |
|              \VV/               |
|   HERE BE (TEMPLATED) DRAGONS   |
|_________________________________|
 |    /\ /      \\       \ /\    |
 |  /   V        ))       V   \  |
 |/     `       //        '     \|
 `              V                '

*/

class RenderingPipeline : NonCopyable {
		template<typename R>
		static R unspecified_func() {
			fatal("Unspecified process function.");
			if constexpr(!std::is_void_v<R>) {
				return Nothing();
			}
		}

		// AnyTuple used to box an argument list (only needed to test for equality)
		struct AnyTuple {
			virtual ~AnyTuple() {
			}

			AnyTuple(const std::type_index& index) : type(index) {
			}

			virtual bool equals(const core::Unique<AnyTuple>& other) = 0;

			std::type_index type;
		};

		template<typename... Args>
		struct Tuple : AnyTuple {
			Tuple(std::tuple<Args...>&& tpl) : AnyTuple(typeid(tuple)), tuple(std::move(tpl)) {
			}

			bool equals(const core::Unique<AnyTuple>& other) override {
				return type == other->type &&
					   tuple == reinterpret_cast<const Tuple<Args...>&>(*other).tuple;
			}

			std::tuple<Args...> tuple;
		};

		template<typename... Args>
		static core::Unique<AnyTuple> make_any_tuple(Args&&... args) {
			if constexpr(sizeof...(Args)) {
				return new Tuple(tuple_maker<Args...>()(std::forward<Args>(args)...));
			} else {
				return new Tuple(std::make_tuple());
			}
		}

		template<typename T, typename... Args>
		struct tuple_maker {
			auto operator()(T&& t, Args&&... args) {
				if constexpr(std::is_reference_v<T>) {
					if constexpr(sizeof...(Args)) {
						return std::tuple_cat(std::make_tuple(&t),
											  tuple_maker<Args...>()(std::forward<Args>(args)...));
					} else {
						return std::make_tuple(&t);
					}
				} else {
					if constexpr(sizeof...(Args)) {
						return std::tuple_cat(std::make_tuple(std::forward<T>(t)),
											  tuple_maker<Args...>()(std::forward<Args>(args)...));
					} else {
						return std::make_tuple(std::forward<T>(t));
					}
				}
			}
		};

		// boxes the future for returning the results
		template<typename T>
		struct ConcurencyData {
			std::promise<T> promise;
			std::shared_future<T> future;

			ConcurencyData() : promise(), future(promise.get_future().share()) {
			}

			ConcurencyData(const ConcurencyData&) {
			}

			ConcurencyData(ConcurencyData&&) = default;
		};

		// box everything necessary to process a node
		struct NodeData {
			std::any concurrency;
			core::Vector<Node*> dependencies;
			core::Function<void()> process = unspecified_func<void>;
			core::Unique<AnyTuple> arguments;
		};

	public:
		template<typename Ret>
		class RenderingNode : NonCopyable {
			public:
				using result_type = Ret;

				template<typename T, typename... Args>
				auto add_dependency(const core::Arc<T>& dep, Args&&... args) {
					_data.dependencies << dep.as_ptr();
					return _pipe.build_frame_graph(dep, std::forward<Args>(args)...);
				}

				template<typename F>
				void set_func(F&& f) {
					using concu_data = ConcurencyData<result_type>;
					_data.process = [cd = std::any_cast<concu_data>(&_data.concurrency),
									 func = std::forward<F>(f)]() mutable {

							if constexpr(std::is_void_v<result_type>) {
								func();
								cd->promise.set_value();
							} else {
								cd->promise. set_value(func());
							}
						};
				}

				const FrameToken& token() const {
					return _pipe.token();
				}

				RenderingPipeline& pipeline() {
					return _pipe;
				}

			private:
				friend class RenderingPipeline;

				RenderingNode(RenderingPipeline& pipe, Node* node, NodeData& data) :
						_pipe(pipe),
						_node(node),
						_data(data) {
				}

				RenderingPipeline& _pipe;

				Node* _node;
				NodeData& _data;
		};


		RenderingPipeline(const FrameToken& token) : _token(token) {
		}

		const FrameToken& token() const {
			return _token;
		}


		template<typename T, typename... Args>
		auto dispatch(const core::Arc<T>& node, Args&&... args) {
			build_frame_graph(node, std::forward<Args>(args)...);
			process_nodes();
		}


	private:
		template<typename T>
		friend class RenderingNode;

		template<typename T>
		struct rendering_node_traits {
			static constexpr bool is_rendering_node = false;
		};

		template<typename T>
		struct rendering_node_traits<RenderingNode<T>> {
			static constexpr bool is_rendering_node = true;
			using result_type = T;
		};

		template<typename T, typename... Args>
		auto build_frame_graph(const core::Arc<T>& node, Args&&... args) {
			using func_type = decltype(T::build_frame_graph);
			using node_type = std::decay_t<typename core::function_traits<func_type>::template arg_type<0>>;
			static_assert(rendering_node_traits<node_type>::is_rendering_node, "T::build_frame_graph does not take a RenderingNode as first argument");
			using result_type = typename rendering_node_traits<node_type>::result_type;
			using concurrency_type = ConcurencyData<result_type>;

			Node* key = node.as_ptr();

			auto it = _nodes.find(key);
			if(it == _nodes.end()) {
				NodeData data;
				data.concurrency.template emplace<concurrency_type>();
				data.arguments = make_any_tuple(args...);

				it = _nodes.insert(std::make_pair(std::move(key), std::move(data))).first;

				node_type rendering_node(*this, node.as_ptr(), it->second);
				node->build_frame_graph(rendering_node, std::forward<Args>(args)...);
			} else {
				if(it->second.arguments->equals(make_any_tuple(args...))) {
					//log_msg(type_name(*node) + " already queued");
				} else {
					log_msg(type_name(*node) + " has already been submitted with different arguments.", Log::Error);
					fatal("A node has been submitted twice with different arguments");
				}
			}

			return std::any_cast<ConcurencyData<typename T::result_type>>(&it->second.concurrency)->future;
		}

		void process_nodes();

		FrameToken _token;
		std::unordered_map<Node*, NodeData> _nodes;
		std::unordered_map<Node*, NodeData> _done;
};


template<typename T>
using RenderingNode = RenderingPipeline::RenderingNode<T>;

}

#endif // YAVE_RENDERERS_RENDERINGPIPELINE_H
