// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/make_copyable.h"
#include "flutter/shell/platform/android/external_view_embedder/surface_pool.h"
#include "flutter/shell/platform/android/jni/jni_mock.h"
#include "flutter/shell/platform/android/surface/android_surface_mock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace flutter {
namespace testing {

using ::testing::ByMove;
using ::testing::Return;

TEST(SurfacePool, GetLayer__AllocateOneLayer) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto jni_mock = std::make_shared<JNIMock>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*jni_mock, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto android_surface_mock = std::make_unique<AndroidSurfaceMock>();
        EXPECT_CALL(*android_surface_mock, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*android_surface_mock, SetNativeWindow(window));
        EXPECT_CALL(*android_surface_mock, IsValid()).WillOnce(Return(true));
        return android_surface_mock;
      };
  auto layer = pool->GetLayer(gr_context.get(), android_context, jni_mock,
                              surface_factory);

  ASSERT_NE(nullptr, layer);
  ASSERT_EQ(reinterpret_cast<intptr_t>(gr_context.get()),
            layer->gr_context_key);
}

TEST(SurfacePool, GetUnusedLayers) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto jni_mock = std::make_shared<JNIMock>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*jni_mock, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto android_surface_mock = std::make_unique<AndroidSurfaceMock>();
        EXPECT_CALL(*android_surface_mock, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*android_surface_mock, SetNativeWindow(window));
        EXPECT_CALL(*android_surface_mock, IsValid()).WillOnce(Return(true));
        return android_surface_mock;
      };
  auto layer = pool->GetLayer(gr_context.get(), android_context, jni_mock,
                              surface_factory);
  ASSERT_EQ(0UL, pool->GetUnusedLayers().size());

  pool->RecycleLayers();

  ASSERT_EQ(1UL, pool->GetUnusedLayers().size());
  ASSERT_EQ(layer, pool->GetUnusedLayers()[0]);
}

TEST(SurfacePool, GetLayer__Recycle) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context_1 = GrContext::MakeMock(nullptr);
  auto jni_mock = std::make_shared<JNIMock>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*jni_mock, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto gr_context_2 = GrContext::MakeMock(nullptr);
  auto surface_factory =
      [gr_context_1, gr_context_2, window](
          std::shared_ptr<AndroidContext> android_context,
          std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto android_surface_mock = std::make_unique<AndroidSurfaceMock>();
        // Allocate two GPU surfaces for each gr context.
        EXPECT_CALL(*android_surface_mock,
                    CreateGPUSurface(gr_context_1.get()));
        EXPECT_CALL(*android_surface_mock,
                    CreateGPUSurface(gr_context_2.get()));
        // Set the native window once.
        EXPECT_CALL(*android_surface_mock, SetNativeWindow(window));
        EXPECT_CALL(*android_surface_mock, IsValid()).WillOnce(Return(true));
        return android_surface_mock;
      };
  auto layer_1 = pool->GetLayer(gr_context_1.get(), android_context, jni_mock,
                                surface_factory);

  pool->RecycleLayers();

  auto layer_2 = pool->GetLayer(gr_context_2.get(), android_context, jni_mock,
                                surface_factory);

  ASSERT_NE(nullptr, layer_1);
  ASSERT_EQ(layer_1, layer_2);
  ASSERT_EQ(reinterpret_cast<intptr_t>(gr_context_2.get()),
            layer_1->gr_context_key);
  ASSERT_EQ(reinterpret_cast<intptr_t>(gr_context_2.get()),
            layer_2->gr_context_key);
}

TEST(SurfacePool, GetLayer__AllocateTwoLayers) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto jni_mock = std::make_shared<JNIMock>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*jni_mock, FlutterViewCreateOverlaySurface())
      .Times(2)
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))))
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              1, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto android_surface_mock = std::make_unique<AndroidSurfaceMock>();
        EXPECT_CALL(*android_surface_mock, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*android_surface_mock, SetNativeWindow(window));
        EXPECT_CALL(*android_surface_mock, IsValid()).WillOnce(Return(true));
        return android_surface_mock;
      };
  auto layer_1 = pool->GetLayer(gr_context.get(), android_context, jni_mock,
                                surface_factory);
  auto layer_2 = pool->GetLayer(gr_context.get(), android_context, jni_mock,
                                surface_factory);
  ASSERT_NE(nullptr, layer_1);
  ASSERT_NE(nullptr, layer_2);
  ASSERT_NE(layer_1, layer_2);
  ASSERT_EQ(0, layer_1->id);
  ASSERT_EQ(1, layer_2->id);
}

TEST(SurfacePool, DestroyLayers) {
  auto pool = std::make_unique<SurfacePool>();
  auto jni_mock = std::make_shared<JNIMock>();

  EXPECT_CALL(*jni_mock, FlutterViewDestroyOverlaySurfaces()).Times(0);
  pool->DestroyLayers(jni_mock);

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*jni_mock, FlutterViewCreateOverlaySurface())
      .Times(1)
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto android_surface_mock = std::make_unique<AndroidSurfaceMock>();
        EXPECT_CALL(*android_surface_mock, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*android_surface_mock, SetNativeWindow(window));
        EXPECT_CALL(*android_surface_mock, IsValid()).WillOnce(Return(true));
        return android_surface_mock;
      };
  pool->GetLayer(gr_context.get(), android_context, jni_mock, surface_factory);

  EXPECT_CALL(*jni_mock, FlutterViewDestroyOverlaySurfaces());
  pool->DestroyLayers(jni_mock);

  pool->RecycleLayers();
  ASSERT_TRUE(pool->GetUnusedLayers().empty());
}

}  // namespace testing
}  // namespace flutter
