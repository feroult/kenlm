#ifndef LM_COUNT_IO__
#define LM_COUNT_IO__

#include <fstream>
#include <string>

#include <err.h>

#include "util/file_piece.hh"

namespace lm {

class CountOutput : boost::noncopyable {
  public:
    explicit CountOutput(const char *name) : file_(name, std::ios::out) {}

    void AddNGram(const StringPiece &line) {
      if (!(file_ << line << '\n')) {
        err(3, "Writing counts file failed");
      }
    }

    template <class Iterator> void AddNGram(const Iterator &begin, const Iterator &end, const StringPiece &line) {
      AddNGram(line);
    }

    void AddNGram(const StringPiece &ngram, const StringPiece &line) {
      AddNGram(line);
    }

  private:
    std::fstream file_;
};

class CountBatch {
  public:
    explicit CountBatch(std::streamsize initial_read) 
      : initial_read_(initial_read) {
      buffer_.reserve(initial_read);
    }

    void Read(std::istream &in) {
      buffer_.resize(initial_read_);
      in.read(&*buffer_.begin(), initial_read_);
      buffer_.resize(in.gcount());
      char got;
      while (in.get(got) && got != '\n')
        buffer_.push_back(got);
    }

    template <class Output> void Send(Output &out) {
      for (util::PieceIterator<'\n'> line(StringPiece(&*buffer_.begin(), buffer_.size())); line; ++line) {
        util::PieceIterator<'\t'> tabber(*line);
        if (!tabber) {
          std::cerr << "Warning: empty n-gram count line being removed\n";
          continue;
        }
        util::PieceIterator<' '> words(*tabber);
        if (!words) {
          std::cerr << "Line has a tab but no words.\n";
          continue;
        }
        out.AddNGram(words, util::PieceIterator<' '>::end(), *line);
      }
    }

  private:
    std::streamsize initial_read_;

    // This could have been a std::string but that's less happy with raw writes.  
    std::vector<char> buffer_;
};

template <class Output> void ReadCount(util::FilePiece &in_file, Output &out) {
  try {
    while (true) {
      StringPiece line = in_file.ReadLine();
      util::PieceIterator<'\t'> tabber(line);
      if (!tabber) {
        std::cerr << "Warning: empty n-gram count line being removed\n";
        continue;
      }
      out.AddNGram(*tabber, line);
    }
  } catch (const util::EndOfFileException &e) {}
}

} // namespace lm

#endif // LM_COUNT_IO__
