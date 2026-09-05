#pragma once

// HINT(alizand):
// Utility base class to explicitly prevent instantiation and copying.
// Inherit from NoInstances to ensure a class cannot be constructed,
// copied, or moved. Useful for static-only classes.
class NoInstances {
 public:
  NoInstances() = delete;
  NoInstances(const NoInstances&) = delete;
  NoInstances(NoInstances&&) = delete;
  NoInstances& operator=(const NoInstances&) = delete;
  NoInstances& operator=(NoInstances&&) = delete;
};
