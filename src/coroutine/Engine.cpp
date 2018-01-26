#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    volatile char StackPointer;
    ctx.Low = ctx.Hight = StackBottom;
    if (&StackPointer > ctx.Low){
        ctx.Hight = const_cast<char*>(&StackPointer);
    } else {
        ctx.Low = const_cast<char*>(&StackPointer);
    }
    int size = ctx.Hight - ctx.Low;
    if (std::get<1>(ctx.Stack) < size) {
        delete[] std::get<0>(ctx.Stack);
        std::get<0>(ctx.Stack) = new char[size];
        std::get<1>(ctx.Stack) = size;
    }
    memcpy(std::get<0>(ctx.Stack), ctx.Low, size);
}

void Engine::Restore(context &ctx) {
    volatile char StackPointer;
    if (ctx.Low <= &StackPointer && &StackPointer <= ctx.Hight) {
        Restore(ctx);
    }
    memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    longjmp(ctx.Environment, 1);
}

void Engine::yield(void) {
    if (cur_routine != nullptr || alive != nullptr) {
        context *ptr;
        for (ptr = alive; ptr && ptr == cur_routine; ptr = ptr->next);
        sched(ptr);
    }
}

void Engine::sched(void *routine_) {
    context *routine = (context *)routine_;
    if (routine != cur_routine) {
        if (routine == nullptr) {
            if (alive == nullptr) {
                return;
            } else {
                routine = alive;
            }
        }
        if (cur_routine) {
            Store(*cur_routine);
            if (setjmp(cur_routine -> Environment)) {
                return;
            }
        }
        cur_routine = routine;
        Restore(*routine);
    }
}

} // namespace Coroutine
} // namespace Afina
