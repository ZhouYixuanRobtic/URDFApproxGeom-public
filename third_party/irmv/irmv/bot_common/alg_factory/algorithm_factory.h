/*
 * Minimal in-repo replacement for the external irmv_core package.
 *
 * AlgorithmFactory: name-keyed registry of std::function factories.
 * AlgorithmRegister + REGISTER_ALGORITHM provide static registration.
 * API-compatible with the original so call sites are unchanged.
 */

#ifndef URDFAPPROXGEOM_IRMV_ALGORITHM_FACTORY_H
#define URDFAPPROXGEOM_IRMV_ALGORITHM_FACTORY_H

#include "irmv/bot_common/alg_factory/bind_this.h"
#include "irmv/bot_common/log/singleton_logger.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace irmv_core {
namespace bot_common {

template <class AlgorithmBase, typename... Args>
class AlgorithmFactory {
public:
    using AlgorithmHash =
        std::unordered_map<std::string, std::function<std::unique_ptr<AlgorithmBase>(Args...)>>;

    static AlgorithmHash& GetAlgorithmHash() {
        static AlgorithmHash algorithm_hash;
        return algorithm_hash;
    }

    template <typename ParamType>
    static bool Register(const std::string algorithm_name, ParamType&& args) {
        AlgorithmHash& algorithm_hash = GetAlgorithmHash();
        auto factory_iter = algorithm_hash.find(algorithm_name);
        if (factory_iter == algorithm_hash.end()) {
            algorithm_hash.emplace(algorithm_name, std::forward<ParamType>(args));
            IRMV_DEBUG("{} registered successfully!", algorithm_name);
        } else {
            IRMV_DEBUG("{} has been registered!", algorithm_name);
        }
        return true;
    }

    static bool UnRegister(const std::string algorithm_name) {
        AlgorithmHash& algorithm_hash = GetAlgorithmHash();
        auto factory_iter = algorithm_hash.find(algorithm_name);
        if (factory_iter != algorithm_hash.end()) {
            algorithm_hash.erase(algorithm_name);
            return true;
        }
        IRMV_DEBUG("Failed to unregister algorithm {}, it is a unregistered algorithm.",
                   algorithm_name);
        return false;
    }

    static std::unique_ptr<AlgorithmBase> CreateAlgorithm(const std::string algorithm_name,
                                                          Args... args) {
        AlgorithmHash& algorithm_hash = GetAlgorithmHash();
        auto factory_iter = algorithm_hash.find(algorithm_name);
        if (factory_iter == algorithm_hash.end()) {
            IRMV_DEBUG("Can't create algorithm {}, because you haven't register it!",
                       algorithm_name);
            return nullptr;
        }
        return (factory_iter->second)(std::forward<Args>(args)...);
    }

private:
    AlgorithmFactory() {}
};

template <typename AlgorithmBase, typename Algorithm, typename... Args>
class AlgorithmRegister {
public:
    explicit AlgorithmRegister(std::string algorithm_name) {
        auto function = [](Args&&... data) {
            return std::make_unique<Algorithm>(std::forward<Args>(data)...);
        };
        AlgorithmFactory<AlgorithmBase, Args...>::Register(algorithm_name, function);
    }

    static std::unique_ptr<Algorithm> create(Args&&... data) {
        return std::make_unique<Algorithm>(std::forward<Args>(data)...);
    }
};

#define NAME(name) register_##name##_algorithm
#define REGISTER_ALGORITHM(AlgorithmBase, AlgorithmName, Algorithm, ...) \
    AlgorithmRegister<AlgorithmBase, Algorithm, ##__VA_ARGS__> NAME(Algorithm)(AlgorithmName)

} // namespace bot_common
} // namespace irmv_core

#endif // URDFAPPROXGEOM_IRMV_ALGORITHM_FACTORY_H
