IMG = python-checker

.PHONY: build run clean

build:
	docker build -t $(IMG) .

run:
	docker run --rm $(IMG)

clean:
	docker rmi -f $(IMG)
