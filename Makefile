all:
	g++ MarketDataClient.cpp -o client -lpthread -lrt
	g++ MarketDataServer.cpp -o server 

