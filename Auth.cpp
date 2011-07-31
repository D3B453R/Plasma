/******************************************************************************
 * This file is part of plNetLog.                                             *
 *                                                                            *
 * plNetLog is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * plNetLog is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with plNetLog.  If not, see <http://www.gnu.org/licenses/>.          *
 ******************************************************************************/

#include "Auth.h"

static const unsigned __int64 kNodeIdx          = (1ull <<  0);
static const unsigned __int64 kCreateTime       = (1ull <<  1);
static const unsigned __int64 kModifyTime       = (1ull <<  2);
static const unsigned __int64 kCreateAgeName    = (1ull <<  3);
static const unsigned __int64 kCreateAgeUuid    = (1ull <<  4);
static const unsigned __int64 kCreatorUuid      = (1ull <<  5);
static const unsigned __int64 kCreatorIdx       = (1ull <<  6);
static const unsigned __int64 kNodeType         = (1ull <<  7);
static const unsigned __int64 kInt32_1          = (1ull <<  8);
static const unsigned __int64 kInt32_2          = (1ull <<  9);
static const unsigned __int64 kInt32_3          = (1ull << 10);
static const unsigned __int64 kInt32_4          = (1ull << 11);
static const unsigned __int64 kUint32_1         = (1ull << 12);
static const unsigned __int64 kUint32_2         = (1ull << 13);
static const unsigned __int64 kUint32_3         = (1ull << 14);
static const unsigned __int64 kUint32_4         = (1ull << 15);
static const unsigned __int64 kUuid_1           = (1ull << 16);
static const unsigned __int64 kUuid_2           = (1ull << 17);
static const unsigned __int64 kUuid_3           = (1ull << 18);
static const unsigned __int64 kUuid_4           = (1ull << 19);
static const unsigned __int64 kString64_1       = (1ull << 20);
static const unsigned __int64 kString64_2       = (1ull << 21);
static const unsigned __int64 kString64_3       = (1ull << 22);
static const unsigned __int64 kString64_4       = (1ull << 23);
static const unsigned __int64 kString64_5       = (1ull << 24);
static const unsigned __int64 kString64_6       = (1ull << 25);
static const unsigned __int64 kIString64_1      = (1ull << 26);
static const unsigned __int64 kIString64_2      = (1ull << 27);
static const unsigned __int64 kText_1           = (1ull << 28);
static const unsigned __int64 kText_2           = (1ull << 29);
static const unsigned __int64 kBlob_1           = (1ull << 30);
static const unsigned __int64 kBlob_2           = (1ull << 31);

static const QString s_nodeTypeNames[] = {
    "Invalid", "VNodeMgrLow", "Player", "Age", "GameServer", "Admin",
    "VaultServer", "CCR", "<8>", "<9>", "<10>", "<11>", "<12>", "<13>",
    "<14>", "<15>", "<16>", "<17>", "<18>", "<19>", "<20>",
    "VNodeMgrHigh", "Folder", "PlayerInfo", "System", "Image", "TextNote",
    "SDL", "AgeLink", "Chronicle", "PlayerInfoList", "UNUSED (31)",
    "Marker", "AgeInfo", "AgeInfoList", "MarkerList"
};

QString readNodeString(ChunkBuffer& buffer)
{
    unsigned length = buffer.read<unsigned>() / sizeof(unsigned short);
    unsigned short* utf16 = new unsigned short[length];
    buffer.chomp(utf16, length * sizeof(unsigned short));
    QString str = QString::fromUtf16(utf16, length-1);
    delete[] utf16;
    return str;
}

void Node_Factory(QTreeWidgetItem* parent, ChunkBuffer& buffer)
{
    unsigned nodeSize = buffer.read<unsigned>();
    if (nodeSize < sizeof(unsigned __int64))
        return;

    unsigned __int64 fields = buffer.read<unsigned __int64>();
    if (fields >> 32)
        OutputDebugStringA("[Node_Factory]\nInvalid bits in node field\n");

    if (fields & kNodeIdx)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Node ID: %1").arg(buffer.read<unsigned>()));
    if (fields & kCreateTime)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Create Time: %1").arg(buffer.read<unsigned>()));
    if (fields & kModifyTime)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Modify Time: %1").arg(buffer.read<unsigned>()));
    if (fields & kCreateAgeName)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Create Age Name: %1").arg(readNodeString(buffer)));
    if (fields & kCreateAgeUuid)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Create Age UUID: %1").arg(buffer.readUuid()));
    if (fields & kCreatorUuid)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Creator UUID: %1").arg(buffer.readUuid()));
    if (fields & kCreatorIdx)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Modify Time: %1").arg(buffer.read<unsigned>()));
    if (fields & kNodeType) {
        unsigned type = buffer.read<unsigned>();
        if (type < (sizeof(s_nodeTypeNames) / sizeof(s_nodeTypeNames[0]))) {
            new QTreeWidgetItem(parent, QStringList()
                << QString("Node Type: %1").arg(s_nodeTypeNames[type]));
        } else {
            new QTreeWidgetItem(parent, QStringList()
                << QString("Node Type: %1").arg(type));
        }
    }
    if (fields & kInt32_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Int32_1: %1").arg(buffer.read<int>()));
    if (fields & kInt32_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Int32_2: %1").arg(buffer.read<int>()));
    if (fields & kInt32_3)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Int32_3: %1").arg(buffer.read<int>()));
    if (fields & kInt32_4)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Int32_4: %1").arg(buffer.read<int>()));
    if (fields & kUint32_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("UInt32_1: %1").arg(buffer.read<unsigned>()));
    if (fields & kUint32_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("UInt32_2: %1").arg(buffer.read<unsigned>()));
    if (fields & kUint32_3)
        new QTreeWidgetItem(parent, QStringList()
            << QString("UInt32_3: %1").arg(buffer.read<unsigned>()));
    if (fields & kUint32_4)
        new QTreeWidgetItem(parent, QStringList()
            << QString("UInt32_4: %1").arg(buffer.read<unsigned>()));
    if (fields & kUuid_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Uuid_1: %1").arg(buffer.readUuid()));
    if (fields & kUuid_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Uuid_2: %1").arg(buffer.readUuid()));
    if (fields & kUuid_3)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Uuid_3: %1").arg(buffer.readUuid()));
    if (fields & kUuid_4)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Uuid_4: %1").arg(buffer.readUuid()));
    if (fields & kString64_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_1: %1").arg(readNodeString(buffer)));
    if (fields & kString64_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_2: %1").arg(readNodeString(buffer)));
    if (fields & kString64_3)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_3: %1").arg(readNodeString(buffer)));
    if (fields & kString64_4)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_4: %1").arg(readNodeString(buffer)));
    if (fields & kString64_5)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_5: %1").arg(readNodeString(buffer)));
    if (fields & kString64_6)
        new QTreeWidgetItem(parent, QStringList()
            << QString("String64_6: %1").arg(readNodeString(buffer)));
    if (fields & kIString64_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("IString64_1: %1").arg(readNodeString(buffer)));
    if (fields & kIString64_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("IString64_2: %1").arg(readNodeString(buffer)));
    if (fields & kText_1)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Text_1: %1").arg(readNodeString(buffer)));
    if (fields & kText_2)
        new QTreeWidgetItem(parent, QStringList()
            << QString("Text_2: %1").arg(readNodeString(buffer)));

    if (fields & kBlob_1) {
        unsigned size = buffer.read<unsigned>();
        new QTreeWidgetItem(parent, QStringList()
            << QString("Blob_1: %1 bytes").arg(size));
        buffer.skip(size);
    }
    if (fields & kBlob_2) {
        unsigned size = buffer.read<unsigned>();
        new QTreeWidgetItem(parent, QStringList()
            << QString("Blob_2: %1 bytes").arg(size));
        buffer.skip(size);
    }
}

bool Auth_Factory(QTreeWidget* logger, QString timeFmt, int direction,
                  ChunkBuffer& buffer)
{
    unsigned short msgId = buffer.read<unsigned short>();

    if (direction == kCli2Srv) {
        switch (msgId) {
        case kCli2Auth_PingRequest:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_PingRequest").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Ping Time: %1 ms").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                size_t payloadSize = buffer.read<unsigned>();
                if (payloadSize > 0) {
                    new QTreeWidgetItem(top, QStringList()
                        << QString("Payload: %1 bytes").arg(payloadSize));
                    buffer.skip(payloadSize);
                    QFont bold = top->font(0);
                    bold.setBold(true);
                    top->setFont(0, bold);
                }
                break;
            }
        case kCli2Auth_ClientRegisterRequest:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_ClientRegisterRequest").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Build ID: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kCli2Auth_AcctLoginRequest:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_AcctLoginRequest").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Challenge: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Account Name: %1").arg(buffer.readString()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Challenge Hash: %1").arg(buffer.readHex<20>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Auth Token: %1").arg(buffer.readString()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("OS: %1").arg(buffer.readString()));
                break;
            }
        case kCli2Auth_AcctSetPlayerRequest:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_AcctSetPlayerRequest").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Player ID: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kCli2Auth_VaultNodeFetch:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_VaultNodeFetch").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Node ID: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kCli2Auth_VaultNodeSave:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_VaultNodeSave").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Node ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Revision ID: %1").arg(buffer.readUuid()));
                QTreeWidgetItem* node = new QTreeWidgetItem(top, QStringList() << "Node");
                Node_Factory(node, buffer);
                break;
            }
        case kCli2Auth_VaultFetchNodeRefs:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_VaultFetchNodeRefs").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Node ID: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kCli2Auth_VaultNodeFind:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_VaultNodeFind").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                QTreeWidgetItem* node = new QTreeWidgetItem(top, QStringList() << "Template Node");
                Node_Factory(node, buffer);
                break;
            }
        case kCli2Auth_AgeRequest:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_AgeRequest").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Age Name: %1").arg(buffer.readString()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Age UUID: %1").arg(buffer.readUuid()));
                break;
            }
        case kCli2Auth_LogPythonTraceback:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Cli2Auth_LogPythonTraceback").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList() << buffer.readString());
                break;
            }
        default:
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 --> Invalid Cli2Auth message (%2)").arg(timeFmt).arg(msgId));
                QFont warnFont = item->font(0);
                warnFont.setBold(true);
                item->setFont(0, warnFont);
                item->setForeground(0, Qt::red);
                OutputDebugStringA(QString("Invalid Cli2Auth message (%1)\n").arg(msgId).toUtf8().data());
                return false;
            }
        }
    } else {
        switch (msgId) {
        case kAuth2Cli_PingReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_PingReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Ping Time: %1 ms").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                size_t payloadSize = buffer.read<unsigned>();
                if (payloadSize > 0) {
                    new QTreeWidgetItem(top, QStringList()
                        << QString("Payload: %1 bytes").arg(payloadSize));
                    buffer.skip(payloadSize);
                    QFont bold = top->font(0);
                    bold.setBold(true);
                    top->setFont(0, bold);
                }
                break;
            }
        case kAuth2Cli_ServerAddr:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_ServerAddr").arg(timeFmt));
                unsigned addr = buffer.read<unsigned>();
                new QTreeWidgetItem(top, QStringList()
                    << QString("Address: %1.%2.%3.%4").arg(addr & 0xFF)
                       .arg((addr >> 8) & 0xFF).arg((addr >> 16) & 0xFF)
                       .arg((addr >> 24) & 0xFF));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Token: %1").arg(buffer.readUuid()));
                top->setForeground(0, kColorAuth);
                break;
            }
        case kAuth2Cli_ClientRegisterReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_ClientRegisterReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Challenge: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kAuth2Cli_AcctLoginReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_AcctLoginReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Account ID: %1").arg(buffer.readUuid()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Account Flags: 0x%1").arg(buffer.read<unsigned>(), 8, 16, QChar('0')));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Billing Type: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Encryption Key: (0x%1, 0x%2, 0x%3, 0x%4)")
                       .arg(buffer.read<unsigned>(), 8, 16, QChar('0'))
                       .arg(buffer.read<unsigned>(), 8, 16, QChar('0'))
                       .arg(buffer.read<unsigned>(), 8, 16, QChar('0'))
                       .arg(buffer.read<unsigned>(), 8, 16, QChar('0')));
                break;
            }
        case kAuth2Cli_AcctPlayerInfo:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_AcctPlayerInfo").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Player ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Player Name: %1").arg(buffer.readString()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Avatar Shape: %1").arg(buffer.readString()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Explorer: %1").arg(buffer.read<unsigned>()));
                break;
            }
        case kAuth2Cli_AcctSetPlayerReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_AcctSetPlayerReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                break;
            }
        case kAuth2Cli_VaultNodeFetched:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_VaultNodeFetched").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                QTreeWidgetItem* node = new QTreeWidgetItem(top, QStringList() << "Node");
                Node_Factory(node, buffer);
                break;
            }
        case kAuth2Cli_VaultNodeChanged:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_VaultNodeChanged").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Node ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Revision ID: %1").arg(buffer.readUuid()));
                break;
            }
        case kAuth2Cli_VaultNodeRefsFetched:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_VaultNodeRefsFetched").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                unsigned count = buffer.read<unsigned>();
                QTreeWidgetItem* refs = new QTreeWidgetItem(top, QStringList() << "Refs");
                for (unsigned i=0; i<count; ++i) {
                    new QTreeWidgetItem(refs, QStringList()
                        << QString("Parent: %1;  Child: %2;  Owner: %3;  [%4]")
                           .arg(buffer.read<unsigned>())
                           .arg(buffer.read<unsigned>())
                           .arg(buffer.read<unsigned>())
                           .arg(buffer.read<bool>() ? "SEEN" : ""));
                }
                break;
            }
        case kAuth2Cli_VaultNodeFindReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_VaultNodeFindReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                unsigned count = buffer.read<unsigned>();
                QTreeWidgetItem* refs = new QTreeWidgetItem(top, QStringList() << "Nodes");
                for (unsigned i=0; i<count; ++i) {
                    new QTreeWidgetItem(refs, QStringList()
                        << QString("Node ID: %1").arg(buffer.read<unsigned>()));
                }
                break;
            }
        case kAuth2Cli_VaultSaveNodeReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_VaultSaveNodeReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                break;
            }
        case kAuth2Cli_AgeReply:
            {
                QTreeWidgetItem* top = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Auth2Cli_AgeReply").arg(timeFmt));
                top->setForeground(0, kColorAuth);
                new QTreeWidgetItem(top, QStringList()
                    << QString("Trans ID: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Result: %1").arg(buffer.readResultCode()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Age MCP: %1").arg(buffer.read<unsigned>()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Instance UUID: %1").arg(buffer.readUuid()));
                new QTreeWidgetItem(top, QStringList()
                    << QString("Vault ID: %1").arg(buffer.read<unsigned>()));
                unsigned addr = buffer.read<unsigned>();
                new QTreeWidgetItem(top, QStringList()
                    << QString("Address: %1.%2.%3.%4").arg(addr & 0xFF)
                       .arg((addr >> 8) & 0xFF).arg((addr >> 16) & 0xFF)
                       .arg((addr >> 24) & 0xFF));
                break;
            }
        default:
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(logger, QStringList()
                    << QString("%1 <-- Invalid Auth2Cli message (%2)").arg(timeFmt).arg(msgId));
                QFont warnFont = item->font(0);
                warnFont.setBold(true);
                item->setFont(0, warnFont);
                item->setForeground(0, Qt::red);
                OutputDebugStringA(QString("Invalid Auth2Cli message (%1)\n").arg(msgId).toUtf8().data());
                return false;
            }
        }
    }

    return true;
}
