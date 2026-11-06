#include "messaging/Dispatcher.h"

void Dispatcher::registerHandler(InternalMessageType type, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[type].push_back(std::move(handler));
}

void Dispatcher::dispatchByType(InternalMessageType type, const std::any& payload) {
    // Copy the handler list out and release the lock before invoking
    // anything: a handler is arbitrary user code that might itself call
    // registerHandler() (deadlock risk) or simply take a while (would
    // otherwise block registerHandler()/other dispatches unnecessarily).
    std::vector<Handler> handlersCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(type);
        if (it == handlers_.end()) return;
        handlersCopy = it->second;
    }
    for (auto& handler : handlersCopy) handler(payload);
}
