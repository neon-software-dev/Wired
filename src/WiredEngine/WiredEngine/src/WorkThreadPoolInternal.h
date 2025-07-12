/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOLINTERNAL_H
#define WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOLINTERNAL_H

#include <NEON/Common/Thread/ResultMessage.h>

#include <functional>
#include <future>

namespace Wired::Engine
{
    struct WorkMessage
    {
        virtual ~WorkMessage() = default;

        virtual void DoWork() = 0;
    };

    template<typename WorkResultT>
    struct WorkMessageImpl : public NCommon::ResultMessage<WorkResultT>, public WorkMessage
    {
        WorkMessageImpl(const std::function<WorkResultT(bool const* isCancelled)>& _workFunc, bool const* _isCancelled)
            : NCommon::ResultMessage<WorkResultT>("WorkMessage"), workFunc(_workFunc), isCancelled(_isCancelled)
        {}

        void DoWork() override
        {
            this->SetResult(workFunc(isCancelled));
        }

        std::function<WorkResultT(bool const*)> workFunc;
        bool const* isCancelled;
    };

    struct NoResultWorkMessageImpl : public NCommon::Message, public WorkMessage
    {
        NoResultWorkMessageImpl(const std::function<void(bool const* isCancelled)>& _workFunc, bool const* _isCancelled)
            : NCommon::Message("WorkMessage"), workFunc(_workFunc), isCancelled(_isCancelled)
        {}

        void DoWork() override
        {
            workFunc(isCancelled);
        }

        std::function<void(bool const*)> workFunc;
        bool const* isCancelled;
    };

    struct WorkEntry
    {
        virtual ~WorkEntry() = default;

        [[nodiscard]] virtual bool TryFulfill() = 0;
    };

    template<typename WorkResultT, typename FuncResultT>
    struct WorkEntryImpl : public WorkEntry
    {
        WorkEntryImpl(std::shared_ptr <WorkMessageImpl<WorkResultT>> workMessage,
                      const std::function<FuncResultT(const WorkResultT&, bool const* isCancelled)>& _resultFunc,
                      bool const* _isCancelled)
            : resultFunc(_resultFunc), isCancelled(_isCancelled)
        {
            messageFuture = workMessage->CreateFuture();
        }

        [[nodiscard]] std::future <FuncResultT> GetFuture()
        {
            return workPromise.get_future();
        }

        [[nodiscard]] bool TryFulfill() override
        {
            if (messageFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                const auto workResult = messageFuture.get();
                const auto funcResult = resultFunc(workResult, isCancelled);
                workPromise.set_value(funcResult);
                return true;
            }

            return false;
        }

        std::function<FuncResultT(const WorkResultT&, bool const*)> resultFunc;
        bool const* isCancelled;
        std::future<WorkResultT> messageFuture;
        std::promise<FuncResultT> workPromise;
    };

    template<typename WorkResultT>
    struct WorkEntryNoReturnImpl : public WorkEntry
    {
        explicit WorkEntryNoReturnImpl(std::shared_ptr <WorkMessageImpl<WorkResultT>> workMessage,
                                       const std::function<void(const WorkResultT&, bool const* isCancelled)>& _resultFunc,
                                       bool const* _isCancelled)
            : resultFunc(_resultFunc), isCancelled(_isCancelled)
        {
            messageFuture = workMessage->CreateFuture();
        }

        [[nodiscard]] bool TryFulfill() override
        {
            if (messageFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                resultFunc(messageFuture.get(), isCancelled);
                return true;
            }

            return false;
        }

        std::function<void(const WorkResultT&, bool const*)> resultFunc;
        bool const* isCancelled;
        std::future <WorkResultT> messageFuture;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOLINTERNAL_H
