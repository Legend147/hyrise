#include "cardinality_estimator_cached.hpp"

#include <chrono>
#include <iostream>

#include "cardinality_cache.hpp"
#include "cardinality_estimator_execution.hpp"

namespace opossum {

CardinalityEstimatorCached::CardinalityEstimatorCached(const std::shared_ptr<CardinalityCache>& cache,
                           const CardinalityEstimationCacheMode cache_mode,
                           const std::shared_ptr<AbstractCardinalityEstimator>& fallback_estimator):
  _cache(cache), _cache_mode(cache_mode), _fallback_estimator(fallback_estimator)
{}

std::optional<Cardinality> CardinalityEstimatorCached::estimate(const std::vector<std::shared_ptr<AbstractLQPNode>>& relations,
                     const std::vector<std::shared_ptr<const AbstractJoinPlanPredicate>>& predicates) const {
  BaseJoinGraph join_graph{relations, predicates};

  const auto cached_cardinality = _cache->get_cardinality(join_graph);

  if (cached_cardinality) return *cached_cardinality;

  /**
   * Use fallbackestimator
   */
  auto fallback_cardinality = std::optional<Cardinality>{};

  if (_fallback_estimator) {
    if (auto estimator_execution = std::dynamic_pointer_cast<CardinalityEstimatorExecution>(_fallback_estimator); estimator_execution) {
      auto previous_timeout = _cache->get_timeout(join_graph);
      if (!previous_timeout || !estimator_execution->timeout || *previous_timeout < *estimator_execution->timeout) {
        fallback_cardinality = _fallback_estimator->estimate(relations, predicates);
      } else {
        std::cout << "CardinalityEstimatorCached: Not executing because it timed out before: " << join_graph.description() << std::endl;
      }
    } else {
      fallback_cardinality = _fallback_estimator->estimate(relations, predicates);
    }

  } else {
    const auto timeout = _cache->get_timeout(join_graph);
    if (timeout) {
      std::cout << "CardinalityEstimatorCached: CardinalityCacheEntry " << join_graph.description() << " has timeout of " << timeout->count() << " - and no fallback estimator specified" << std::endl;
    } else {
      std::cout << "CardinalityEstimatorCached: Cardinality for " << join_graph.description() << " not in cache - and no fallback estimator specified" << std::endl;
    }
  }

  if (fallback_cardinality) {
    if (_cache_mode == CardinalityEstimationCacheMode::ReadAndUpdate) {
      _cache->set_cardinality({relations, predicates}, fallback_cardinality.value(), 0);
    }
  } else if (auto estimator_execution = std::dynamic_pointer_cast<CardinalityEstimatorExecution>(_fallback_estimator);
             estimator_execution) {
    _cache->set_timeout(join_graph, estimator_execution->timeout);
  }

  return fallback_cardinality;
}

}  // namespace opossum