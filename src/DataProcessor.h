#ifndef PROJECT_DATAPROCESSOR_H
#define PROJECT_DATAPROCESSOR_H

#include <string>
#include <fstream>
#include <memory>


class DataProcessor {
  std::ifstream file;

  std::string filename;
  int chunk_size;

  int file_size;

  std::unique_ptr<char[]> current_chunk;


  int full_chunks_qty;
  int last_chunk_size;
  int total_chunks_qty;

  int chunk_index;
  int current_chunk_size;


public:
  explicit DataProcessor(const std::string &filename, int chunk_size);

  ~DataProcessor();

  int remainingChunks();

  char *readChunk();

  char *getLastChunk();

  int getCurrentChunkSize();

  void clearChunk();

};


#endif //PROJECT_DATAPROCESSOR_H
