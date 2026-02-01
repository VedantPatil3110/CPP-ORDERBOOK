#include<bits/stdc++.h>
#include<format>
//enum is a special data type used to define a fixed set of named constants.
using namespace std;
    enum class OrderType{
        GoodTillCancel, //Place this order and keep it until I cancel it or it gets fully filled.
        //Goes into bids_ / asks_
        // Can be partially filled
        // Stays in the book after partial fills
        // Removed only when:
        // fully filled, or
        // CancelOrder() is called

        FillAndKill,//Execute immediately as much as possible, cancel whatever remains.
        //Tries to match immediately
        // Partial fills allowed
        // Remaining quantity is cancelled
        // Never rests in the book

        FillOrKill,//Either execute the entire order immediately, or do nothing.
        //Pre-checks available liquidity
        // Partial fills NOT allowed
        // If full quantity not available → cancel entire order
        // Never rests in the book

        GoodForDay//Keep this order until the end of the trading day.
        //Behaves like GTC during the day
        // Can be partially filled
        // Remains in the book
        // Automatically cancelled by:CancelGoodForDayOrders();
    };

    enum class Side{
        buy,
        sell
    };

    using Price=int;
    using Quantity=uint32_t;
    using OrderId=uint64_t;
    
    struct LevelInfo  //class type bana diya hai idhar
    {
        Price price_;
        Quantity quantity_;
    };

    using LevelInfos=vector<LevelInfo>; //woh class ke andar ke hisse ka use 
    class OrderBookLevelInfos{
        // Bid = someone wants to BUY
        // Ask = someone wants to SELL
        private:
        LevelInfos bids_;
        LevelInfos asks_;
        public:
        OrderBookLevelInfos(const LevelInfos& bids,const LevelInfos &asks){
        bids_=bids;
        asks_=asks;
        }
    };
    
// Create an order for the orderbook start of the trade 
    class Order{
        private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;

        public:
        Order(OrderType orderType,OrderId orderId,Side side,Price price,Quantity quantity){
            orderType_=orderType;
            orderId_=orderId;
            side_=side;
            price_=price;
            initialQuantity_=quantity;
            remainingQuantity_=quantity;
        }
        OrderType GetOrderType(){
            return orderType_;
        }
        OrderId GetOrderId(){
            return orderId_;
        }
        Side GetSide(){
            return side_;
        }
        Price GetPrice(){
            return price_;
        }
        Quantity getInitialQuantity(){
            return initialQuantity_;
        }
        Quantity getRemainingQuantity(){
            return remainingQuantity_;
        }
        Quantity GetFilledQuantity(){
            return getInitialQuantity()-getRemainingQuantity();
        }
        bool IsFilled(){
            return getRemainingQuantity()==0;
        }
        void Fill(Quantity quantity){
            if(quantity>getRemainingQuantity()){
                throw logic_error("order cannot be filled for more than its remaining quantity. "+ to_string(GetOrderId()));
            }
            remainingQuantity_-=quantity;
        }
    };

using OrderPointer=shared_ptr<Order>; //smart ptr that automatically manage memory
// One Order object can be referenced from multiple places safely. 
using OrderPointers=list<OrderPointer>; //popfront o(1) time complexity mai ho jata hai toh easy hai

// Command Request to modify like user wants to change an order.
// cancel old order->create a fresh order->insert it again
class OrderModify{
    private:
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity quantity_;
    public:
    OrderModify(OrderId orderId,Side side,Price price,Quantity quantity){
        orderId_=orderId;
        price_=price;
        side_=side;
        quantity_=quantity;
    }
    OrderId GetOrderId(){
        return orderId_;
    }
    Side GetSide(){
        return side_;
    }
    Price GetPrice(){
        return price_;
    }
    Quantity GetQuantity(){
        return quantity_;
    }
    OrderPointer ToOrderPointer(OrderType type){
        return make_shared<Order>(type,GetOrderId(),GetSide(),GetPrice(),GetQuantity());
    }
};

// when an order is matched 
// trade obj used trade info for bid and ask 
// using struct here only diff memebers are public by default 
struct TradeInfo{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;

};
// agregation of tradeinfo->bid side and ask side trade 
class Trade{
    private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
    public:
    Trade(const TradeInfo &bidTrade,const TradeInfo &askTrade){  //used const here to ensure constructor cannot modify the passed tradeinfo objects making the code safer 
        bidTrade_=bidTrade;
        askTrade_=askTrade;
    }
    TradeInfo GetBidTrade(){
        return bidTrade_;
    }
    TradeInfo GetAskTrade(){
        return askTrade_;
    }
};
//Whenever you write Trades, the compiler treats it as vector<Trade>.
using Trades=vector<Trade>;


//  real work is done here
class Orderbook{
    // store order we think of 2 ds maps and unordered maps we use maps to represent bids and ask bids aare sorted in desc for best bid and ask are sorted in asc for best ask.
    private:
    struct OrderEntry{
        //order_=variable that holds a pointer to an order object.
        OrderPointer order_{nullptr}; //identity
        OrderPointers::iterator location_; //location
        //store position of an order inside a vector 
    };
    map<Price,OrderPointers,greater<Price>>bids_; //sorted asec high price first
    map<Price,OrderPointers,less<Price>>asks_; //sorted asec low price first 
    unordered_map<OrderId,OrderEntry>orders_;
    // add 2 methods match and canmatch->fillandkill if it does not match it just discard there it helps

    //It tells you whether an order can match immediately against the current order book.
    bool canMatch(Side side,Price price){
        // scope resolution operator for enum class
        if(side==Side::buy){ //buy order has arrived
            // user wants to buy we check asks_ map to check for the lowest price 
            if(asks_.empty()){ //if empty just return false 
                return false;
            }
            auto it=asks_.begin();
            Price bestAsk=it->first;
            return price>=bestAsk;//if price is greater than or equal to best ask just return true in budget
        }
        else{//sell order has arrived 
            // user wants to sell so we check bids map to find best bid to sell at 
            if(bids_.empty()){
                return false;
            }
            auto it=bids_.begin();
            Price bestBid=it->first;
            return price<=bestBid; //if price is lower or equal to best bid just return true will create profit benificial trade 
        }
    }
    //To match buy orders (bids) with sell orders (asks) and generate trades.
    Trades MatchOrders(){
        Trades trades; //create empty vector to store trade objects
        trades.reserve(orders_.size()); //pre allocates the memory in vector 
        while (true){ //keep trying to match until no more matches avilable 
            if(bids_.empty()|| asks_.empty()){ //if either empty just stop as no buyers or sellers 
                break;
            }
            auto bidIt=bids_.begin();
            auto askIt=asks_.begin();
            Price bidPrice=bidIt->first;//bestbid
            Price askPrice=askIt->first;//bestask
            OrderPointers& bids = bidIt->second;
            OrderPointers& asks = askIt->second;
            if(bidPrice<askPrice){//buyer not willing to pay enuf
                break;
            }
            while (!bids.empty() && !asks.empty()){//orders at this price keep matching
                auto bid=bids.front();
                auto ask=asks.front();
                Quantity quantity=min(bid->getRemainingQuantity(),ask->getRemainingQuantity());
                bid->Fill(quantity); //check fill ie error->quantity>remainingquantity or remainingquantity-quantity
                ask->Fill(quantity); //same as above for this
                if(bid->IsFilled()){ //if remainingquantity==0
                    bids.pop_front();
                    orders_.erase(bid->GetOrderId());
                }
                if(ask->IsFilled()){ //if remainingquantity==0
                    asks.pop_front();
                    orders_.erase(ask->GetOrderId());
                }
                if(bids.empty()){//if empty remove the bid
                    bids_.erase(bidPrice);
                }
                if(asks.empty()){//if emptyt remove the ask
                    asks_.erase(askPrice);
                }
                trades.push_back(Trade{  //trade vector maipush kardo bid aur ask (unki id,unki currvalue,quantity)
                    TradeInfo{bid->GetOrderId(),bid->GetPrice(),quantity},
                    TradeInfo{ask->GetOrderId(),ask->GetPrice(),quantity}
                });
            }
        }
        //handle left over fill and kill orders
        if(!bids_.empty()){//to check if leftover buy
            auto bidIt=bids_.begin();
            OrderPointers &bids=bidIt->second;
            if(!bids.empty()){
                auto order=bids.front();
                if(order->GetOrderType()==OrderType::FillAndKill){ //fill and kill property fill immidiately and cancel whatever remains
                    CancelOrder(order->GetOrderId());
                }
            }
        }
        if(!asks_.empty()){ //same as above to check if leftover ask
            auto askIt=asks_.begin();
            OrderPointers &asks=askIt->second;
            if(!asks.empty()){
                auto order=asks.front();
                if(order->GetOrderType()==OrderType::FillAndKill){ //similarly cancel the best ask if fill and kill
                    CancelOrder(order->GetOrderId());
                }
            }
        }
        return trades;
    }
    //this dunction is to decide whether fill or kill should execute or not
    //fill or kill either fill to quantity or do nothing
    bool CanFullyFill(OrderPointer order){
        Quantity remaining=order->getRemainingQuantity();
        Price price=order->GetPrice();
        Side side=order->GetSide();
        if(side==Side::buy){
            for(auto &[askPrice,orders]:asks_){
                if(askPrice>price){ //if askprice>price just do nothing and break return false;
                    break;
                }
                for(auto &ask:orders){
                    remaining-=min(remaining,ask->getRemainingQuantity()); //else find remaining and subtract 
                    if(remaining==0){ //if remain is 0 then full filled fill or kill can execute 
                        return true;
                    }
                }
            }
        }
        else{
            for(auto &[bidPrice,orders]:bids_){
                if(bidPrice<price){//if bidprice<price just do nothing and break return false;
                    break;
                }
                for(auto &bid:orders){
                    remaining-=min(remaining,bid->getRemainingQuantity()); //find remaining and subtract 
                    if(remaining==0){ //if remain is 0 then full filled fill or kill can execute 
                        return true;
                    }
                }
            }
        }
        return false;
    }
    public:
    Trades AddOrder(OrderPointer order){
        if(orders_.find(order->GetOrderId())!=orders_.end()){ //if order avilabe in orders_ map then return nothing as order already in process
            return {};
        }
        if(order->GetOrderType()==OrderType::FillOrKill){ //here we check from can fully fill as fillor kill needs to finish the order immediately or just ignore 
            if(!CanFullyFill(order)){
                return{};
            }
        }
        //here we check fill and kill with can can match as canmatch if false tells no immidiate execution and fill and kill means complete the order immidiately or cancel the order so just return an empty vector as process cannot go further 
        if(order->GetOrderType()==OrderType::FillAndKill && !canMatch(order->GetSide(),order->GetPrice())){
            return {};
        }
        OrderPointers::iterator it; //from order pointer we take a iterator it to check prev and next order 
        //itterator tells where the order is stored at that moment of time 
        if(order->GetSide()==Side::buy){ //check first what is the side
            OrderPointers &orders=bids_[order->GetPrice()];//then find order pointer for that order if order not exsit empty list
            orders.push_back(order); //push the order in list at back
            it=prev(orders.end());//iterator that points to the previous element orders pointing to end so before that.
        }
        else{
            //same as above for asks
            OrderPointers &orders=asks_[order->GetPrice()];
            orders.push_back(order);
            it=prev(orders.end());
        }
        //save orderid and push in orders_
        orders_.insert({order->GetOrderId(),OrderEntry{order,it}});
        return MatchOrders();//everything in matchorders happen and it returns ans 
    }
    //it is to remove exsiting order completely from orderbook
    void CancelOrder(OrderId orderId){
        if(orders_.find(orderId)==orders_.end()){ //if orders_does not have the order then return nothing
            return;
        }
        auto entry=orders_.at(orderId);
        OrderPointer order=entry.order_; //order object
        auto it=entry.location_;//exact position of order in bids and ask
        orders_.erase(orderId); //remove from orders_ map
        if(order->GetSide()==Side::sell){
            auto price=order->GetPrice();//obtain price 
            auto &orders=asks_.at(price);//find order in asks_ map
            orders.erase(it); //remove the exact pos in bids and asks
            if(orders.empty()){ //no sell orders left at the price remove the price list from the asks_ map
                asks_.erase(price);// keep order book correct and clean
            }
        }
        //same as above no change
        else{
            auto price=order->GetPrice();
            auto &orders=bids_.at(price);
            orders.erase(it);
            if(orders.empty()){
                bids_.erase(price);
            }
        }
    }
    //this is for
    //cancel the old order->create a new order with updated vals->insert and match normally
    //this is done to avoid confusion simplify the complexity and make the opperation go smoothly
    Trades MatchOrder(OrderModify order){
        if(orders_.find(order.GetOrderId())==orders_.end()){ //if orders_ does not have order just return 
            return {};
        }
        auto entry=orders_.at(order.GetOrderId());
        OrderPointer existingOrder=entry.order_;
        CancelOrder(order.GetOrderId()); //cancel old order here 
        return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType())); //for rematching send it back to addorder function
    }
    //returns the size of orders_ ie no of order remain
    std::size_t Size(){
        return orders_.size();
    }
    //this function is to give summarized view 
    //it gives a simple snapshot
    OrderBookLevelInfos GetOrderInfos(){
        LevelInfos bidInfos,askInfos; //
        // reserve() pre-allocates memory for a vector
        bidInfos.reserve(orders_.size()); //just tooptimize and make operations smooth giving already the size 
        askInfos.reserve(orders_.size());
        auto CreateLevelInfos=[](Price price,OrderPointers &orders){ //lambda function created just for the local scope 
            Quantity TotalQuantity=0;//here we take a total quantity variable and itterate over order at the same price in list to find total quantity at that price 
            for(auto &order:orders){
                TotalQuantity+=order->getRemainingQuantity();
            }
            return LevelInfo{price,TotalQuantity}; //return the price of order and give the totalquantity at that price 
        };
        //bids summary
        for(auto &[price,orders]:bids_){
            bidInfos.push_back(CreateLevelInfos(price,orders));//give a snapshot of price and orders at same price
        }
        for(auto &[price,orders]:asks_){
            askInfos.push_back(CreateLevelInfos(price,orders));//give a snapshot of price and orders at same price
        }
        return OrderBookLevelInfos{bidInfos,askInfos};
    }
    //for cancelgoodforday
    //stays and act as good till cancel but at end of day just cancels the order whatever done is done
    void CancelGoodForDayOrders(){
        vector<OrderId>toCancel;
        for(auto &[orderId,entry]:orders_){ //iterate over orders_ just to find order with goodforday and the cancels it
            if(entry.order_->GetOrderType()==OrderType::GoodForDay){
                toCancel.push_back(orderId);
            }
        }
        for(OrderId id:toCancel){
            CancelOrder(id);
        }
    }
};
int main() {
    Orderbook orderbook;
    cout << "Initial size: " << orderbook.Size() << endl;
    // Add GTC buy order
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodTillCancel,
            1,
            Side::buy,
            100,
            10
        )
    );
    // Add GFD sell order (no match yet)
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodForDay,
            2,
            Side::sell,
            105,
            10
        )
    );
    cout << "After adding 2 orders: " << orderbook.Size() << endl;
    //Add GFD buy order (matches order 2)
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodForDay,
            3,
            Side::buy,
            110,
            10
        )
    );
    cout << "After matching GFD orders: " << orderbook.Size() << endl;
    //Add Fill-And-Kill order (executes immediately)
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::FillAndKill,
            4,
            Side::sell,
            99,
            5
        )
    );
    cout << "After FillAndKill order: " << orderbook.Size() << endl;
    //Cancel a specific order
    orderbook.CancelOrder(1);
    cout << "After cancelling order 1: " << orderbook.Size() << endl;
    //Add liquidity for Fill-Or-Kill test
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodTillCancel,
            7,
            Side::sell,
            102,
            6
        )
    );
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodTillCancel,
            8,
            Side::sell,
            108,
            4
        )
    );
    cout << "Before FillOrKill order: " << orderbook.Size() << endl;
    //Fill-Or-Kill buy order (must fully execute or cancel)
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::FillOrKill,
            9,
            Side::buy,
            110,
            10
        )
    );
    cout << "After FillOrKill order: " << orderbook.Size() << endl;
    //Add Good-For-Day orders
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodForDay,
            10,
            Side::buy,
            95,
            10
        )
    );
    orderbook.AddOrder(
        make_shared<Order>(
            OrderType::GoodForDay,
            11,
            Side::sell,
            120,
            10
        )
    );
    cout << "Before end-of-day cleanup: " << orderbook.Size() << endl;
    //Simulate end of trading day
    orderbook.CancelGoodForDayOrders();
    cout << "After end-of-day cleanup: " << orderbook.Size() << endl;
    return 0;
}
