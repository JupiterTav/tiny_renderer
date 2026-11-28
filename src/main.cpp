#include "Application.hpp"

int main(int argc, char *argv[]) {
  Application app(argc > 0 ? argv[0] : nullptr);
  return app.run();
}
