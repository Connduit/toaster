//#ifndef DISPATCHER_H
//#define DISPATCHER_H
//
//#include <any>
//#include <functional>
//#include <mutex>
//#include <unordered_map>
//#include <vector>
//
////#include "messaging/InternalMessageType.h"
//
//// Routes typed messages from producers to whichever handlers registered
//// for that message's type.
////
//// Replaces an earlier design where a single Pipeline fanned every raw
//// IQ buffer out to every registered stage indiscriminately. That's
//// wrong once stages want different kinds of data (a signal classifier
//// might want post-demod audio, not raw IQ) or have very different
//// costs (a slow stage shouldn't force every other stage, or the RF read
//// thread, to wait on it). Dispatcher lets a producer publish a message
//// without knowing who -- if anyone -- is listening, and lets a handler
//// subscribe to exactly the message type(s) it cares about without
//// knowing who else is listening or where the message came from.
////
//// The public dispatch<MessageT>() / registerTypedHandler<MessageT>()
//// are templated for convenient, type-checked call sites -- the compiler
//// infers the message type from the argument you pass or the template
//// argument you give, and callers never touch std::any directly.
//// Internally, handlers are stored type-erased (via std::any) and keyed
//// by the runtime InternalMessageType enum, which is what lets one
//// Dispatcher hold handlers for arbitrarily many message types without a
//// new method needing to be added to this class every time a new message
//// type is introduced.
////
//// Thread-safety: registerHandler()/dispatch() may be called from
//// different threads (e.g. the RF read thread dispatching IQMessages
//// concurrently with a console-listener thread dispatching a
//// CommandMessage) and are internally synchronized. Handlers are copied
//// out and invoked without holding the lock, so a handler is free to
//// register another handler without deadlocking. What this does *not*
//// protect against: two dispatches of the *same* message type from two
//// different threads at the same moment would still invoke that type's
//// handlers concurrently with each other, so a handler that isn't itself
//// thread-safe should only ever be fed from one thread. Every message
//// type in this project currently has exactly one producer thread, which
//// is what makes that safe today -- worth re-checking if that ever
//// changes.
//class Dispatcher {
//public:
//    using Handler = std::function<void(const std::any& payload)>;
//
//    // Registers a type-erased handler directly. Prefer
//    // registerTypedHandler() below unless you have a specific reason to
//    // work with std::any yourself.
//    void registerHandler(InternalMessageType type, Handler handler);
//
//    // Registers a handler for a specific message type without the
//    // caller ever touching std::any.
//    template <typename MessageT>
//    void registerTypedHandler(std::function<void(const MessageT&)> handler) {
//        registerHandler(MessageT::kType,
//                         [handler = std::move(handler)](const std::any& payload) {
//                             handler(std::any_cast<const MessageT&>(payload));
//                         });
//    }
//
//    // Dispatches a message to every handler registered for its type
//    // (MessageT::kType). A no-op if nothing is registered for that type.
//    template <typename MessageT>
//    void dispatch(const MessageT& message) {
//        dispatchByType(MessageT::kType, message);
//    }
//
//private:
//    void dispatchByType(InternalMessageType type, const std::any& payload);
//
//    std::mutex mutex_;
//    std::unordered_map<InternalMessageType, std::vector<Handler>> handlers_;
//};
//
////#include "ToasterTypes.h"
////
////#include <cstdint>
////#include <complex>
////#include <functional>
////#include <unordered_map>
////#include <vector>
////
////class Dispatcher
////{
////public:
////    //using IQData = IQData;
////    using Handler = std::function<void(const IQData&)>;
////
////    // Type would be an enum representing the component i want to send the iq data too
////    void registerHandler(int type, Handler handler);
////    //void registerHandler(Handler handler);
////    void dispatch(const uint8_t* buf, uint32_t len);
////    void dispatch(const IQData& iqData);
////
////private:
////    std::unordered_map<int, Handler> handlers_;
////};
////
////
//////#include <functional>
//////
//////class Dispatcher
//////{
//////public:
//////    using Handler = std::function<void(const InternalMessage&)>;
//////    void registerHandler(MessageType type, Handler handler);
//////
//////	// TODO: only need one of these. find out which one is needed and remove the others
//////    void dispatch(const InternalMessage& msg);
//////    void dispatch(InternalMessage& msg);
//////private:
//////    std::unordered_map<MessageType, Handler> handlers_;
//////};
//
//#endif