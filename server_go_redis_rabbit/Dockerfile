# syntax=docker/dockerfile:1

FROM --platform=$BUILDPLATFORM golang:1.27-alpine AS builder

ARG TARGETOS
ARG TARGETARCH

WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .

RUN CGO_ENABLED=0 \
    GOOS=$TARGETOS \
    GOARCH=$TARGETARCH \
    go build -o /out/api ./cmd/api

RUN CGO_ENABLED=0 \
    GOOS=$TARGETOS \
    GOARCH=$TARGETARCH \
    go build -o /out/worker ./cmd/worker

FROM alpine:latest

WORKDIR /app

COPY --from=builder --chmod=755 /out/api /app/api
COPY --from=builder --chmod=755 /out/worker /app/worker

EXPOSE 8080

CMD ["/app/api"]