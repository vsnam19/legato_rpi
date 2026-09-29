#include <gtest/gtest.h>
#include "cfgRouter.hpp"

using namespace cfg;

TEST(CfgRouterTest, ResolveValidRoutes) {
    CfgRouter router;

    // RAW route in Network domain
    uint32_t idRaw = MakeId(Category::Raw, Subsystem::Network, 42);
    auto routeRaw = router.ResolveRoute(idRaw);
    ASSERT_TRUE(routeRaw.has_value());
    EXPECT_EQ(routeRaw->category, Category::Raw);
    EXPECT_EQ(routeRaw->internalKey, "net/item_42");
    EXPECT_EQ(routeRaw->GetFullConfigTreePath(), "/cfgManager/net/item_42");
    EXPECT_FALSE(routeRaw->isReadOnly);
    EXPECT_FALSE(routeRaw->isVolatile);

    // SENSITIVE route in Security domain with ReadOnly flag
    uint32_t idSens = MakeId(Category::Sensitive, Subsystem::Security, 7, Flag::ReadOnly);
    auto routeSens = router.ResolveRoute(idSens);
    ASSERT_TRUE(routeSens.has_value());
    EXPECT_EQ(routeSens->category, Category::Sensitive);
    EXPECT_EQ(routeSens->internalKey, "sec/item_7");
    EXPECT_EQ(routeSens->GetFullConfigTreePath(), "/cfgManager/sec/item_7");
    EXPECT_TRUE(routeSens->isReadOnly);
    EXPECT_FALSE(routeSens->isVolatile);

    // SECURE route in System domain with Volatile flag
    uint32_t idSec = MakeId(Category::Secure, Subsystem::System, 100, Flag::Volatile);
    auto routeSec = router.ResolveRoute(idSec);
    ASSERT_TRUE(routeSec.has_value());
    EXPECT_EQ(routeSec->category, Category::Secure);
    EXPECT_EQ(routeSec->internalKey, "sys/item_100");
    EXPECT_FALSE(routeSec->isReadOnly);
    EXPECT_TRUE(routeSec->isVolatile);

    // App domain
    uint32_t idApp = MakeId(Category::Raw, Subsystem::App, 1);
    auto routeApp = router.ResolveRoute(idApp);
    ASSERT_TRUE(routeApp.has_value());
    EXPECT_EQ(routeApp->internalKey, "app/item_1");
}

TEST(CfgRouterTest, RejectInvalidCfgIds) {
    CfgRouter router;

    // Bad ID: Conflicting category flags
    uint32_t badId = ((static_cast<uint32_t>(Category::Raw) | static_cast<uint32_t>(Category::Sensitive)) << 24) | (0x01 << 16) | 1;
    EXPECT_FALSE(router.ResolveRoute(badId).has_value());

    // Bad ID: Invalid subsystem
    uint32_t badSub = (static_cast<uint32_t>(Category::Raw) << 24) | (0x7F << 16) | 1;
    EXPECT_FALSE(router.ResolveRoute(badSub).has_value());
}

TEST(CfgRouterTest, CustomMapping) {
    CfgRouter router;
    uint32_t id = MakeId(Category::Sensitive, Subsystem::Network, 10);

    EXPECT_TRUE(router.RegisterCustomKey(id, "net/wifi_credentials"));
    auto route = router.ResolveRoute(id);
    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(route->internalKey, "net/wifi_credentials");
    EXPECT_EQ(route->GetFullConfigTreePath(), "/cfgManager/net/wifi_credentials");

    // Invalid ID custom key registration fails
    uint32_t badId = 0;
    EXPECT_FALSE(router.RegisterCustomKey(badId, "invalid"));

    // Empty custom key registration fails
    EXPECT_FALSE(router.RegisterCustomKey(id, ""));
}

