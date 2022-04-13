docker build --build-arg IMAGE=arm32v7/alpine:3.15 -t ghcr.io/nyamisty/altserver_builder_alpine_armv7 .
docker push ghcr.io/nyamisty/altserver_builder_alpine_armv7

docker build --build-arg IMAGE=arm64v8/alpine:3.15 -t ghcr.io/nyamisty/altserver_builder_alpine_aarch64 .
docker push ghcr.io/nyamisty/altserver_builder_alpine_aarch64

docker build --build-arg IMAGE=amd64/alpine:3.15 -t ghcr.io/nyamisty/altserver_builder_alpine_amd64 .
docker push ghcr.io/nyamisty/altserver_builder_alpine_amd64

docker build --build-arg IMAGE=i386/alpine:3.15 -t ghcr.io/nyamisty/altserver_builder_alpine_i386 .
docker push ghcr.io/nyamisty/altserver_builder_alpine_i386
