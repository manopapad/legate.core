/* Copyright 2021-2022 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "core/task/task.h"

namespace legate {

using namespace Legion;

void LegateTaskRegistrar::record_variant(TaskID tid,
                                         const char* task_name,
                                         const CodeDescriptor& descriptor,
                                         ExecutionConstraintSet& execution_constraints,
                                         TaskLayoutConstraintSet& layout_constraints,
                                         LegateVariantCode var,
                                         Processor::Kind kind,
                                         const VariantOptions& options)
{
  assert((kind == Processor::LOC_PROC) || (kind == Processor::TOC_PROC) ||
         (kind == Processor::OMP_PROC));

  auto handle = [&](Processor::Kind _kind, LegateVariantCode _var, const char* var_name) {
    // Buffer these up until we can do our actual registration with the runtime
    pending_task_variants_.push_back(PendingTaskVariant(
      tid, false /*global*/, var_name, task_name, descriptor, _var, options.return_size));

    auto& registrar = pending_task_variants_.back();
    registrar.execution_constraints.swap(execution_constraints);
    registrar.layout_constraints.swap(layout_constraints);
    registrar.add_constraint(ProcessorConstraint(_kind));
    registrar.set_leaf(options.leaf);
    registrar.set_inner(options.inner);
    registrar.set_idempotent(options.idempotent);
    registrar.set_concurrent(options.concurrent);
  };

  handle(kind,
         var,
         (kind == Processor::LOC_PROC)   ? "CPU"
         : (kind == Processor::TOC_PROC) ? "GPU"
                                         : "OpenMP");
  if (kind == Processor::LOC_PROC) handle(Processor::PY_PROC, (legate_core_variant_t)4, "PY");
}

void LegateTaskRegistrar::register_all_tasks(Runtime* runtime, LibraryContext& context)
{
  // Do all our registrations
  for (auto& task : pending_task_variants_) {
    task.task_id =
      context.get_task_id(task.task_id);  // Convert a task local task id to a global id
    // Attach the task name too for debugging
    runtime->attach_name(task.task_id, task.task_name, false /*mutable*/, true /*local only*/);
    runtime->register_task_variant(task, task.descriptor, nullptr, 0, task.ret_size, task.var);
  }
  pending_task_variants_.clear();
}

}  // namespace legate
