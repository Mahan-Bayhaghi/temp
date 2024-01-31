#include "cifar10_reader.hpp"

auto dataset = cifar::read_dataset<std::vector, std::vector, uint8_t, uint8_t>();