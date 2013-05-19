#define NUM_OF_PACKETS 1000
#define DEBUG_LOG_FILE "MarketDebugClient.log"
enum ExchangeA_MsgType {NewLevel,DeleteLevel,ModifyLevel};

struct ExchangeA_MD {
	uint16_t seqno_; // sequence no- will be monotonically increasing for
									 // successive packets
	char contract_[4]; // name of security
	uint8_t level_; // top of book has level 0 and level increases sequentially.
	double price_;
	uint16_t size_;
	char side_; // B for buy side, A for sell side
	enum ExchangeA_MsgType msg_;
};
