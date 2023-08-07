// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <iostream>

#include "vmt.h"

namespace {
    class DummyBase {
    public:
        virtual DECLSPEC_NOINLINE bool DummyTarget() noexcept {
            std::cout << "DummyBase::DummyTarget() is called...\n";
            return true;
        }

        virtual DECLSPEC_NOINLINE bool DummyTarget2() noexcept {
            std::cout << "DummyBase::DummyTarget2() is called...\n";
            return true;
        }

        virtual DECLSPEC_NOINLINE bool DummyTarget3() noexcept {
            std::cout << "DummyBase::DummyTarget3() is called...\n";
            return true;
        }
    };

    class DummyDerived : public DummyBase {
    public:
        virtual DECLSPEC_NOINLINE bool DummyTarget() noexcept override {
            std::cout << "DummyDerived::DummyTarget() is called...\n";
            return true;
        }

        virtual DECLSPEC_NOINLINE bool DummyTarget2() noexcept {
            std::cout << "DummyDerived::DummyTarget2() is called...\n";
            return true;
        }
    };

    class DummyDerived2 : public DummyBase {
    public:
        virtual DECLSPEC_NOINLINE bool DummyTarget() noexcept override {
            std::cout << "DummyDerived2::DummyTarget() is called...\n";
            return true;
        }
    };

    bool DECLSPEC_NOINLINE DetouredTarget(DummyBase* inst) noexcept {
        std::cout << "Intercepted and detoured DummyTarget call from instance: 0x" << ::std::hex << inst << "\n";
        std::cout << "DetouredTarget() is called...\n";
        return false;
    }
}

TEST(winhookupp_test, winhookupp_test_vmt)
{
    using namespace WINHOOKUPP_NM;

    Vmt vmt;

    DummyDerived derived;
    LPVOID origin;

    DummyBase* pDummy = &derived;

    // could possibly be a vcall thunk
    auto x = &DummyDerived::DummyTarget;
    LPVOID target = (LPVOID&)x;

    ASSERT_EQ(pDummy->DummyTarget(), true);
    ASSERT_EQ(vmt.Enable(target, &::DetouredTarget, pDummy, &origin), true);
    ASSERT_EQ(pDummy->DummyTarget(), false);
    ASSERT_EQ(vmt.Disable(), true);
    ASSERT_EQ(pDummy->DummyTarget(), true);
}