FROM openwrt/rootfs:x86-64-23.05.3

# Rootfs OpenWRT rất minimal
# opkg cần các thư mục này
RUN mkdir -p /var/lock /var/lib/opkg /tmp

# Copy file .ipk vào image
COPY ipk/check-python_*.ipk /tmp/

# Cài package local bằng opkg
RUN opkg install /tmp/check-python_*.ipk && \
    rm /tmp/check-python_*.ipk

# Mở shell interactive khi container start
CMD ["/bin/sh"]
