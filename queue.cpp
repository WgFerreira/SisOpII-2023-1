#include "include/queue.h"

void MessageQueue::push(message m) 
{
  this->mutex_write.lock();
    this->queue.push_back(m);
  this->mutex_write.unlock();

  this->mutex_read.unlock();
}

void MessageQueue::push(list<message> m) 
{
  this->mutex_write.lock();
    this->queue.splice(this->queue.end(), m);
  this->mutex_write.unlock();

  this->mutex_read.unlock();
}

message MessageQueue::pop() 
{
  message m;
  this->mutex_write.lock();
    m = this->queue.front();
    this->queue.pop_front();
  this->mutex_write.unlock();

  this->mutex_read.unlock();
  this->mutex_read.lock();
  return m;
}

std::string messageTypeToString(MessageType type)
{
  switch (type)
  {
  case MessageType::MANAGER_ELECTION :
    return "MANAGER_ELECTION";
  case MessageType::ELECTION_ANSWER :
    return "ELECTION_ANSWER";
  case MessageType::ELECTION_VICTORY :
    return "ELECTION_VICTORY";
  case MessageType::STATUS_REQUEST :
    return "STATUS_REQUEST";
  case MessageType::STATUS_RESPONSE :
    return "STATUS_RESPONSE";
  case MessageType::LEAVING :
    return "LEAVING";
  
  default:
    return "NONE";
  } 
}

void OperationQueue::push(table_operation m) 
{
  this->mutex_write.lock();
    this->queue.push_back(m);
  this->mutex_write.unlock();

  this->mutex_read.unlock();
}

void OperationQueue::push(list<table_operation> m) 
{
  this->mutex_write.lock();
    this->queue.splice(this->queue.end(), m);
  this->mutex_write.unlock();

  this->mutex_read.unlock();
}

table_operation OperationQueue::pop() 
{
  this->mutex_write.lock();
    table_operation m = this->queue.front();
    this->queue.pop_front();
  this->mutex_write.unlock();

  this->mutex_read.unlock();
  this->mutex_read.lock();
  return m;
}
