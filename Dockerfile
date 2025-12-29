FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    sqlite3 \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY libs/ ./libs/
COPY src/ ./src/
COPY database/ ./database/
COPY Makefile ./

# Build the server
RUN make server

# Create database directory if it doesn't exist
RUN mkdir -p database

# Expose port 5500
EXPOSE 5500

# Run the server on port 5500
CMD ["./server", "5500"]
