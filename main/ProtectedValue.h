#pragma once

#include <chrono>
#include <cstdint>
#include <random>

namespace Memory {
    class ProtectedInt {
        public:
            ProtectedInt() {
                Set(0);
            }

            explicit ProtectedInt(int value) {
                Set(value);
            }

            void Set(int value) {
                key_ = GenerateKey();
                encodedValue_ = static_cast<std::uint32_t>(value) ^ key_;
                verifier_ = MakeVerifier(encodedValue_, key_);
            }

            int Get() const {
                return IsValid() ? static_cast<int>(encodedValue_ ^ key_) : 0;
            }

            bool IsValid() const {
                return verifier_ == MakeVerifier(encodedValue_, key_);
            }

        private:
            static std::uint32_t GenerateKey() {
                std::random_device randomDevice;
                const auto timeValue = static_cast<std::uint32_t>(
                    std::chrono::steady_clock::now().time_since_epoch().count());
                const std::uint32_t key = randomDevice() ^ timeValue;
                return key != 0 ? key : 0xA5C39E17u;
            }

            static std::uint32_t MakeVerifier(std::uint32_t encodedValue, std::uint32_t key) {
                std::uint32_t value = encodedValue ^ (key + 0x9E3779B9u);
                value = (value << 13) | (value >> 19);
                return value ^ 0x85EBCA6Bu;
            }

            std::uint32_t encodedValue_ = 0;
            std::uint32_t key_ = 0;
            std::uint32_t verifier_ = 0;
    };
}
