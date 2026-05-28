IMG = check-python-builder
ARTIFACT_DIR = artifacts

.PHONY: all build package clean

all: build


# Build Docker image (compile C + sinh .ipk)
build:
	docker build -t $(IMG) .


# Chạy container, mount artifacts/ vào /out/
# CMD trong container sẽ copy .ipk ra ngoài host
package: build
	mkdir -p $(ARTIFACT_DIR)
	docker run --rm -v $(PWD)/$(ARTIFACT_DIR):/out $(IMG)

	@echo "----------------------------------------"
	@echo "Artifacts:"
	@ls -lh $(ARTIFACT_DIR)/


# Dọn dẹp
clean:
	rm -rf $(ARTIFACT_DIR)/*
	docker rmi -f $(IMG) 2>/dev/null || true

	@echo "Cleaned."
