IMG = check-python-runtime

.PHONY: all build run clean check-ipk

all: build


# Kiểm tra có file .ipk chưa
check-ipk:
	@ls ipk/check-python_*.ipk 2>/dev/null || \
		(echo 'ERROR: Không tìm thấy .ipk trong ipk/' && \
		echo 'Hãy copy file từ mock_build/artifacts sang mock_test/ipk/' && \
		exit 1)


# Build runtime image
build: check-ipk
	docker build -t $(IMG) .


# Chạy shell interactive trong OpenWRT container
run:
	docker run --rm -it $(IMG)


# Dọn dẹp
clean:
	docker rmi -f $(IMG) 2>/dev/null || true

	@echo "Cleaned."
