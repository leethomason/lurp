#pragma once

#include "TaskScheduler.h"

namespace lurp {

struct CompletionActionDelete : enki::ICompletable
{
    enki::Dependency _dependency;

    void OnDependenciesComplete(enki::TaskScheduler* pTaskScheduler_, uint32_t threadNum_)
    {
        // Call base class OnDependenciesComplete BEFORE deleting depedent task or self
        ICompletable::OnDependenciesComplete(pTaskScheduler_, threadNum_);

        // I find this very roundabout. But this is the approach used in the enkiTS examples.
        delete _dependency.GetDependencyTask();
    }
};

struct SelfDeletingTask : enki::ITaskSet
{
    SelfDeletingTask()
    {
        _taskDeleter.SetDependency(_taskDeleter._dependency, this);
    }
    virtual ~SelfDeletingTask() {}

    CompletionActionDelete _taskDeleter;
    enki::Dependency _dependency;
};

} // namespace lurp

