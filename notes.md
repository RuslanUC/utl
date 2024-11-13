# This is just my notes for me about implementation

### MessageDef & FieldDef structure
Example:
```
---types---
peerUser#59511722 user_id:long = Peer;
messageEmpty#90a6ca84 flags:# id:int peer_id:flags.0?Peer = Message;
chatEmpty#29562865 id:long = Chat;
userEmpty#d3bc4b7a id:long = User;
messages.messages#8c718e87 messages:Vector<Message> chats:Vector<Chat> users:Vector<User> = messages.Messages;
...
// LAYER 177
```
- MessageDef (ptr MessageDef#1)
  - id - 0x8c718e87
  - name - "messages"
  - namespace_ - "messages"
  - type - "Messages"
  - typespace - "messages"
  - section - "types"
  - has_optional - false
  - layer - 177
  - fields_num - 3
  - fields:
    - FieldDef:
      - num - 0
      - name - "messages"
      - type - utl_FieldType::VECTOR
      - flag_info - 0
      - sub_message_def - (ptr MessageDefVector#2)
    - FieldDef:
      - num - 1
      - name - "chats"
      - type - utl_FieldType::VECTOR
      - flag_info - 0
      - sub_message_def - (ptr MessageDefVector#5)
    - FieldDef:
      - num - 2
      - name - "users"
      - type - utl_FieldType::VECTOR
      - flag_info - 0
      - sub_message_def - (ptr MessageDefVector#7)


- MessageDefVector (ptr MessageDefVector#2)
  - type - utl_FieldType::TLOBJECT
  - flag_info - 0
  - sub_message_def - (ptr MessageDef#3)


- MessageDef (ptr MessageDef#3)
  - id - 0x90a6ca84
  - name - "messageEmpty"
  - namespace_ - ""
  - type - "Message"
  - typespace - ""
  - section - "types"
  - has_optional - true
  - layer - 177
  - fields_num - 3
  - fields:
    - FieldDef:
      - num - 0
      - name - "flags"
      - type - utl_FieldType::FLAGS
      - flag_info - 0
      - sub_message_def - (ptr 0x00)
    - FieldDef:
      - num - 1
      - name - "id"
      - type - utl_FieldType::INT32
      - flag_info - 0
      - sub_message_def - (ptr 0x00)
    - FieldDef:
      - num - 2
      - name - "peer_id"
      - type - utl_FieldType::TLOBJECT
      - flag_info - 0b00100000
      - sub_message_def - (ptr MessageDef#4)


- MessageDef (ptr MessageDef#4)
  - id - 0x59511722
  - name - "peerUser"
  - namespace_ - ""
  - type - "Peer"
  - typespace - ""
  - section - "types"
  - has_optional - true
  - layer - 177
  - fields_num - 1
  - fields:
    - FieldDef:
      - num - 0
      - name - "user_id"
      - type - utl_FieldType::INT64
      - flag_info - 0
      - sub_message_def - (ptr 0x00)


- MessageDefVector (ptr MessageDefVector#5)
  - type - utl_FieldType::TLOBJECT
  - flag_info - 0
  - sub_message_def - (ptr MessageDef#6)


- MessageDef (ptr MessageDef#6)
  - id - 0x29562865
  - name - "chatEmpty"
  - namespace_ - ""
  - type - "Chat"
  - typespace - ""
  - section - "types"
  - has_optional - false
  - layer - 177
  - fields_num - 1
  - fields:
    - FieldDef:
      - num - 0
      - name - "id"
      - type - utl_FieldType::INT64
      - flag_info - 0
      - sub_message_def - (ptr 0x00)


- MessageDefVector (ptr MessageDefVector#7)
  - type - utl_FieldType::TLOBJECT
  - flag_info - 0
  - sub_message_def - (ptr MessageDef#8)


- MessageDef (ptr MessageDef#8)
  - id - 0xd3bc4b7a
  - name - "userEmpty"
  - namespace_ - ""
  - type - "User"
  - typespace - ""
  - section - "types"
  - has_optional - false
  - layer - 177
  - fields_num - 1
  - fields:
    - FieldDef:
      - num - 0
      - name - "id"
      - type - utl_FieldType::INT64
      - flag_info - 0
      - sub_message_def - (ptr 0x00)
