#include <qendian.h>

#include "vlan.h"

VlanConfigForm *VlanProtocol::configForm = NULL;

VlanConfigForm::VlanConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

VlanProtocol::VlanProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new VlanConfigForm;
}

VlanProtocol::~VlanProtocol()
{
}

AbstractProtocol* VlanProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new VlanProtocol(frameProtoList, streamCore);
}

void VlanProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::vlan)->CopyFrom(data);
}

void VlanProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::vlan))
		data.MergeFrom(stream.GetExtension(OstProto::vlan));
}

QString VlanProtocol::name() const
{
	return QString("Vlan");
}

QString VlanProtocol::shortName() const
{
	return QString("Vlan");
}

int	VlanProtocol::fieldCount() const
{
	return vlan_fieldCount;
}

AbstractProtocol::FieldFlags VlanProtocol::fieldFlags(int index) const
{
	AbstractProtocol::FieldFlags flags;

	flags = AbstractProtocol::fieldFlags(index);

	switch (index)
	{
		case vlan_tpid:
		case vlan_prio:
		case vlan_cfiDei:
		case vlan_vlanId:
			break;

		// meta-fields
		case vlan_isOverrideTpid:
			flags |= FieldIsMeta;
			break;
	}

	return flags;
}

QVariant VlanProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case vlan_tpid:
		{
			quint16 tpid;

			tpid = data.is_override_tpid() ? data.tpid() : 0x8100;

			switch(attrib)
			{
				case FieldName:			
					return QString("Tag Protocol Id");
				case FieldValue:
					return tpid;
				case FieldTextValue:
					return QString("0x%1").arg(tpid, 2, BASE_HEX, QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian(tpid, (uchar*) fv.data()); 
					return fv;
				}
				default:
					break;
			}
			break;
		}

		case vlan_prio:
		{
			uint prio = ((data.vlan_tag() >> 13) & 0x07);

			switch(attrib)
			{
				case FieldName:			
					return QString("Priority");
				case FieldValue:
					return prio;
				case FieldTextValue:
					return QString("%1").arg(prio);
				case FieldFrameValue:
					return QByteArray(1, (char) prio);
				case FieldBitSize:
					return 3;
				default:
					break;
			}
			break;
		}

		case vlan_cfiDei:
		{
			uint cfiDei = ((data.vlan_tag() >> 12) & 0x01); 

			switch(attrib)
			{
				case FieldName:			
					return QString("CFI/DEI");
				case FieldValue:
					return cfiDei;
				case FieldTextValue:
					return QString("%1").arg(cfiDei);
				case FieldFrameValue:
					return QByteArray(1, (char) cfiDei);
				case FieldBitSize:
					return 1;
				default:
					break;
			}
			break;
		}

		case vlan_vlanId:
		{
			quint16 vlanId = (data.vlan_tag() & 0x0FFF);

			switch(attrib)
			{
				case FieldName:			
					return QString("VLAN Id");
				case FieldValue:
					return vlanId;
				case FieldTextValue:
					return QString("%1").arg(vlanId);
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) vlanId, (uchar*) fv.data()); 
					return fv;
				}
				case FieldBitSize:
					return 12;
				default:
					break;
			}
			break;
		}
		// Meta fields

		case vlan_isOverrideTpid:
		default:
			break;
	}

	return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool VlanProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* VlanProtocol::configWidget()
{
	return configForm;
}

void VlanProtocol::loadConfigWidget()
{
	configForm->leTpid->setText(uintToHexStr(fieldData(vlan_tpid, FieldValue).toUInt(), 2));
	configForm->cmbPrio->setCurrentIndex(fieldData(vlan_prio, FieldValue).toUInt());
	configForm->cmbCfiDei->setCurrentIndex(fieldData(vlan_cfiDei, FieldValue).toUInt());
	configForm->leVlanId->setText(fieldData(vlan_vlanId, FieldValue).toString());

}

void VlanProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_is_override_tpid(configForm->cbTpidOverride->isChecked());
	data.set_tpid(configForm->leTpid->text().remove(QChar(' ')).toULong(&isOk, BASE_HEX));
	data.set_vlan_tag(
		((configForm->cmbPrio->currentIndex()   & 0x07) << 13) |
		((configForm->cmbCfiDei->currentIndex() & 0x01) << 12) |
		(configForm->leVlanId->text().toULong(&isOk) & 0x0FFF));
}

