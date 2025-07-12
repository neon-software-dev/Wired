/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOL_H
#define WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOL_H

#include "WorkThreadPoolInternal.h"

#include <NEON/Common/Thread/MessageDrivenThreadPool.h>
#include <NEON/Common/Thread/ResultMessage.h>
#include <NEON/Common/Thread/ThreadUtil.h>

#include <memory>
#include <functional>
#include <future>
#include <vector>

namespace Wired::Engine
{
    /**
     * Thread pool which executes submitted work functions asynchronously. Some Submit methods will also
     * execute a result function on the engine thread once the corresponding work function has finished.
     *
     * Work and result functions are provided an isCancelled boolean pointer which can/should be
     * checked when possible in order to stop work early if the work has been cancelled.
     */
    class WorkThreadPool
    {
        public:

            explicit WorkThreadPool(unsigned int numThreads);
            ~WorkThreadPool();

            /**
             * Executes workFunc on a pool thread.
             */
            void Submit(const std::function<void(bool const* isCancelled)>& workFunc)
            {
                m_threadPool->PostMessage(std::make_shared<NoResultWorkMessageImpl>(workFunc, &m_canceled));
            }

            /**
             * Executes workFunc on a pool thread. Returns a future to track the work. If the future
             * is destructed with the work still active, it does not block.
             */
            template <typename WorkResultT>
            [[nodiscard]] std::future<WorkResultT> SubmitForResult(const std::function<WorkResultT(bool const* isCancelled)>& workFunc)
            {
                auto workMessage = std::make_shared<WorkMessageImpl<WorkResultT>>(workFunc, &m_canceled);
                auto future = workMessage->CreateFuture();
                m_threadPool->PostMessage(workMessage);
                return future;
            }

            /**
             * Executes workFunc on a pool thread, and executes resultFunc on the engine thread when
             * workFunc has finished.
             */
            template <typename WorkResultT>
            void SubmitFinishedOnMain(
                const std::function<WorkResultT(bool const* isCancelled)>& workFunc,
                const std::function<void(const WorkResultT&, bool const* isCancelled)>& resultFunc)
            {
                auto workMessage = std::make_shared<WorkMessageImpl<WorkResultT>>(workFunc, &m_canceled);
                auto workEntry = std::make_shared<WorkEntryNoReturnImpl<WorkResultT>>(workMessage, resultFunc, &m_canceled);

                m_finishedOnMainEntries.emplace_back(workEntry);
                m_threadPool->PostMessage(workMessage);
            }

            /**
             * Executes workFunc on a pool thread, and executes resultFunc on the engine thread when
             * workFunc has finished. Returns a future to track the work. If the future is destructed
             * with the work still active, it does not block.
             *
             * WARNING! The future returned by this must NOT be waited for on the engine thread, as the
             * engine thread needs to be running in order for resultFunc to be executed and the work finished.
             */
            template <typename WorkResultT, typename FuncResultT>
            [[nodiscard]] std::future<FuncResultT> SubmitFinishedOnMainForResult(
                const std::function<WorkResultT(bool const* isCancelled)>& workFunc,
                const std::function<FuncResultT(const WorkResultT&, bool const* isCancelled)>& resultFunc)
            {
                auto workMessage = std::make_shared<WorkMessageImpl<WorkResultT>>(workFunc, &m_canceled);
                auto workEntry = std::make_shared<WorkEntryImpl<WorkResultT, FuncResultT>>(workMessage, resultFunc, &m_canceled);
                auto workFuture = workEntry->GetFuture();

                m_finishedOnMainEntries.emplace_back(workEntry);
                m_threadPool->PostMessage(workMessage);

                return workFuture;
            }

            /**
             * Process previously submitted FinishedOnMain work which has finished. Executes their
             * resultFunc and then erases state tracking the work. This method should only be
             * called from the engine thread.
             */
            void PumpFinished();

        private:

            static void HandleMessage(const NCommon::Message::Ptr& message);

        private:

            std::unique_ptr<NCommon::MessageDrivenThreadPool> m_threadPool;
            std::vector<std::shared_ptr<WorkEntry>> m_finishedOnMainEntries;
            bool m_canceled{false};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORKTHREADPOOL_H
