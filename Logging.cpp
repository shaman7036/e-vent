#include "Logging.h"


namespace logging {


/// Var

template <typename T>
Var::Var(const String& label, T* var, const int& min_digits, const int& float_precision):
    label_(label),
    min_digits_(min_digits),
    float_precision_(float_precision) {
  setPtr(var);
}


String Var::serialize() const {
  switch (type_) {
    case INT:
      return serialize(var_.i);
    case FLOAT:
      return serialize(var_.f);
    case DOUBLE:
      return serialize(var_.d);
  }
}

void Var::setPtr(int* var) {
  var_.i = var;
  type_ = INT;
}

void Var::setPtr(float* var) {
  var_.f = var;
  type_ = FLOAT;
}

void Var::setPtr(double* var) {
  var_.d = var;
  type_ = DOUBLE;
}

String Var::serialize(int* var) const {
  char buff[kMaxChars];
  snprintf(buff, kMaxChars, "%d", *var);

  String string_out(buff);
  while (string_out.length() < min_digits_) {
    string_out = " " + string_out;
  }

  return String(buff);
}

String Var::serialize(float* var) const {
  char buff[kMaxChars];
  dtostrf(*var, min_digits_, float_precision_, buff);
  return String(buff);
}

String Var::serialize(double* var) const {
  char buff[kMaxChars];
  dtostrf(*var, min_digits_, float_precision_, buff);
  return String(buff);
}


/// Logger

Logger::Logger(bool log_to_serial, bool log_to_SD, 
               bool serial_labels, const String delim):
    log_to_serial_(log_to_serial),
    log_to_SD_(log_to_SD),
    serial_labels_(serial_labels),
    delim_(delim) {}

template <typename T>
void Logger::addVar(const char var_name[], T* var, 
                    const int& min_digits, const int& float_precision) {
  vars_[num_vars_++] = Var(var_name, var, min_digits, float_precision);
}

void Logger::begin(const Stream* serial, const int& pin_select_SD) {
  stream_ = serial;

  if (log_to_SD_) {
    pinMode(pin_select_SD, OUTPUT);

    if (!SD.begin(pin_select_SD)) {
      stream_->println("SD card initialization failed!");
      return;
    }

    makeFile();
  }
}

void Logger::update() {
  if ((!log_to_serial_ && !log_to_SD_) || num_vars_ == 0) {
    return;
  }

  String line, line_with_labels;

  const bool need_line_without_labels = log_to_SD_ || (log_to_serial_ && !serial_labels_);
  const bool need_line_with_labels = log_to_serial_ && serial_labels_;

  for (int i = 0; i < num_vars_; i++) {
    String word = vars_[i].serialize();
    if (i != num_vars_ - 1) {
      word += delim_;
    }
    if (need_line_without_labels) {
      line += word;
    }
    if (need_line_with_labels) {
      line_with_labels += vars_[i].label() + ": " + word;
    }
  }
  
  if (log_to_serial_) {
    stream_->println(serial_labels_ ? line_with_labels : line);
  }

  if (log_to_SD_) {
    File file = SD.open(filename_, FILE_WRITE);
    file.println(line);
    file.close();
  }
}

void Logger::makeFile() {
  // setup SD card data logger
  File number_file = SD.open("number.txt", FILE_READ);

  int num;
  if(number_file){
    num = number_file.parseInt();  

    number_file.close();
  }

  SD.remove("number.txt");
  
  number_file = SD.open("number.txt", FILE_WRITE);

  if(number_file){
    number_file.println(num+1);

    number_file.close();
  }

  snprintf(filename_, sizeof(filename_), "DATA%03d.TXT", num);

  if(stream_->available()) {
    stream_->print("DATA FILE NAME: ");
    stream_->println(filename_);
  }
  
  File file = SD.open(filename_, FILE_WRITE);
  if (file) {
    if(stream_->available()) {
      stream_->print("Writing to ");
      stream_->print(filename_);
      stream_->println("...");
    }
    String line;
    for (int i = 0; i < num_vars_; i++) {
      line += vars_[i].label();
      if (i != num_vars_ - 1) {
        line += delim_;
      }
    }
    file.println(line);
    file.close();
  } else {
    if(stream_->available()) {
      // if the file didn't open, print an error:
      stream_->print("error opening ");
      stream_->println(filename_);
    }
    // else throw an SD card error!
  }
}


}  // namespace logging

