include $(TOPDIR)/rules.mk

PKG_NAME:=hello
PKG_VERSION:=1.0.0
PKG_DESC:=hello example for C programms

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_UNPACK:=cp -rf src/* $(PKG_BUILD_DIR)

PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/Install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hello $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/sdk/
	$(INSTALL_BIN) files/sdk.sh $(1)/etc/sdk/$(PKG_NAME)
endef

$(eval $(call BuildPackage))
