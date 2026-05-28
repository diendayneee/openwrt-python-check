include $(TOPDIR)/rules.mk

PKG_NAME := check-python
PKG_VERSION := 1.0
PKG_RELEASE := 1

include $(INCLUDE_DIR)/package.mk


define Package/check-python
	SECTION := utils
	CATEGORY := Utilities
	TITLE := Check Python 3.9 version utility
endef


define Package/check-python/description
	A C utility that checks for Python 3.9 installation
	and logs the version to /tmp/python_ver.log
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/check_python \
		$(PKG_BUILD_DIR)/check_python.c
endef


define Package/check-python/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/check_python $(1)/usr/bin/
endef


$(eval $(call BuildPackage,check-python))
