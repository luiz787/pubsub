#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <iostream>

using namespace std;

typedef string Client;
typedef string Tag;

class MessageBroker
{
private:
  map<Client, set<Tag>> controlMap;

public:
  MessageBroker()
  {
    this->controlMap = map<Client, set<Tag>>();
  }

  void subscribe(Client client, Tag subject)
  {
    if (isSubscribed(client, subject))
    {
      auto message = "already subscribed +" + subject;
      throw invalid_argument(message);
    }
    controlMap[client].emplace(subject);
  }

  bool isSubscribed(Client client, Tag subject)
  {
    auto element = controlMap[client].find(subject);
    return element != controlMap[client].end();
  }

  void unsubscribe(Client client, Tag subject)
  {
    if (!isSubscribed(client, subject))
    {
      auto message = "not subscribed +" + subject;
      throw invalid_argument(message);
    }

    controlMap[client].erase(subject);
  }

  void onMessage(string msg)
  {
    auto tags = getTags(msg);
    publish(msg, tags);
  }

  void publish(string msg, set<Tag> tags)
  {
    for (auto client : controlMap)
    {
      auto tagsOfInterest = client.second;

      // debugPrint(client);

      vector<Tag> intersection;
      set_intersection(tagsOfInterest.begin(), tagsOfInterest.end(), tags.begin(), tags.end(), std::back_inserter(intersection));

      // If the intersection between the set of tags that the client is interested in and the set of tags in the message
      // is not empty, that means the client is interested in at least one of the message's tags.
      if (!intersection.empty())
      {
        send(client.first, msg);
      }
    }
  }

  void debugPrint(pair<Client, set<Tag>> clientData)
  {
    cout << "Client " << clientData.first << endl;
    cout << "Tags of interest:" << endl;
    for (auto tag : clientData.second)
    {
      cout << tag << endl;
    }
  }

  void send(string client, string msg) const
  {
    cout << client << " would receive the following message: \"" << msg << "\"" << endl;
    // TODO: send msg to client.
  }

  set<Tag> getTags(string msg) const
  {
    istringstream iss(msg);
    vector<string> tokens{
        istream_iterator<string>{iss},
        istream_iterator<string>{}};

    set<Tag> tags = set<Tag>();

    for (auto token : tokens)
    {
      if (token[0] == '#')
      {
        Tag tag = getTagFromToken(token);
        tags.emplace(tag);
      }
    }
    return tags;
  }

  Tag getTagFromToken(Tag token) const
  {
    Tag tag = "";

    for (size_t i = 0; i < token.size(); i++)
    {
      auto c = token[i];

      if (isalpha((int)c))
      {
        tag += c;
      }
    }
    return tag;
  }
};

int main()
{

  MessageBroker broker;
  broker.unsubscribe("Luiz", "pipoca");

  return 0;
}
