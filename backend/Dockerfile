FROM japeto/so-tools:v64

WORKDIR /app
COPY *.cpp *.h ./
RUN g++ -std=c++11 -O2 *.cpp -o server -pthread
EXPOSE 5000

CMD ["./server"]
