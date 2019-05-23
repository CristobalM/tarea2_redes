#include "DataProcessor.h"

#include <algorithm>
#include <iostream>

int getFileSize(const std::string &filename) {
  std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);
  file.seekg(0, std::ios::end);
  return (int) file.tellg();
}

DataProcessor::DataProcessor(const std::string &filename, int chunk_size) : filename(filename), chunk_size(chunk_size) {
  std::cout << "Trying to read filename " << filename << std::endl;
  file = std::ifstream(filename);
  if (!file.good()) {
    throw std::runtime_error("File " + filename + " does not exist");
  }

  current_chunk = std::make_unique<char[]>(chunk_size + 1);
  clearChunk();

  file_size = getFileSize(filename);

  full_chunks_qty = file_size / chunk_size;
  last_chunk_size = file_size % chunk_size;
  total_chunks_qty = full_chunks_qty + (last_chunk_size > 0 ? 1 : 0);

  chunk_index = 0;
  current_chunk_size = 0;
}

char *DataProcessor::readChunk() {
  if (chunk_index == total_chunks_qty) {
    return nullptr;
  }

  clearChunk();
  if (chunk_index == full_chunks_qty) {
    current_chunk_size = last_chunk_size;
  } else {
    current_chunk_size = chunk_size;
  }
  file.read(current_chunk.get(), current_chunk_size);

  chunk_index++;

  return current_chunk.get();
}

char *DataProcessor::getLastChunk() {
  return current_chunk.get();
}

inline void DataProcessor::clearChunk() {
  std::fill(current_chunk.get(), current_chunk.get() + chunk_size + 1, 0);
}

int DataProcessor::getCurrentChunkSize() {
  return current_chunk_size;
}

DataProcessor::~DataProcessor() {
  if (file) {
    file.close();
  }
}

int DataProcessor::remainingChunks() {
  return total_chunks_qty - chunk_index;
}

