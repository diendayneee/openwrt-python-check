FROM openwrt/sdk:x86-64-23.05.3

# SDK image đã có sẵn buildroot environment

# Setup feeds (lần đầu cần internet)
RUN ./scripts/feeds update -a && \
    ./scripts/feeds install -a && \
    make defconfig

# Copy package definition vào SDK
COPY openwrt-pkt/Makefile package/check-python/Makefile
COPY openwrt-pkt/src package/check-python/src

# Compile package
RUN make package/check-python/compile V=s

# Khi container chạy -> copy .ipk ra /out
CMD cp bin/packages/x86_64/base/check-python_*.ipk /out/
