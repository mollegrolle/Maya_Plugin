#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>


#define PLUGINNAME "[MayaApi]"

using namespace std;
MCallbackId callbackId;
MCallbackIdArray callbackIdArray;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

enum NODE_TYPE { TRANSFORM, MESH };
MTimer gTimer;

// keep track of created meshes to maintain them
queue<MObject> newMeshes;

// Maya command once
// commandPort -n ":1234"

/*
 * how Maya calls this method when a node is added.
 * new POLY mesh: kPolyXXX, kTransform, kMesh
 * new MATERIAL : kBlinn, kShadingEngine, kMaterialInfo
 * new LIGHT    : kTransform, [kPointLight, kDirLight, kAmbientLight]
 * new JOINT    : kJoint
 */
void nodeAddedFn(MObject& node, void* clientData);
void nodeRemovedFn(MObject& node, void* clientData);
void nameChangedFn(MObject& node, const MString& str, void* clientData);
void polyTopologyChanged(MObject& node, void* clientData);
void timerFn(float elapsedTime, float lastTime, void* clientData);
void attributeChangedFn(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);
void appendCallbackFn(string name, MCallbackId* id, MStatus* status);
void addCallbacksFn();

void nodeAddedFn(MObject& node, void* clientData) {

	MFnDependencyNode nodeFn(node, &status);

	MString nameStr = PLUGINNAME;
	nameStr += " - Node Added (";
	nameStr += node.apiTypeStr();
	nameStr += "): '";
	nameStr += nodeFn.name();
	nameStr += "' ";
	nameStr += status.errorString();

	MGlobal::displayInfo(nameStr);

	callbackId = MNodeMessage::addNameChangedCallback(node, nameChangedFn, NULL, &status);
	appendCallbackFn("addNameChangedCallback", &callbackId, &status);

	callbackId = MNodeMessage::addAttributeChangedCallback(node, attributeChangedFn, kDefaultNodeType, &status);
	appendCallbackFn("addAttributeChangedCallback", &callbackId, &status);

	//callbackId = MPolyMessage::addPolyTopologyChangedCallback(node, polyTopologyChanged, kDefaultNodeType, &status);
	//appendCallbackFn("addAttributeChangedCallback", &callbackId, &status);

}

void nodeRemovedFn(MObject& node, void* clientData) {

	MFnDependencyNode nodeFn(node);

	MString nameStr = PLUGINNAME;
	nameStr += " - Node Removed (";
	nameStr += node.apiTypeStr();
	nameStr += "): '";
	nameStr += nodeFn.name();
	nameStr += "'";

	MGlobal::displayInfo(nameStr);
}

void nameChangedFn(MObject& node, const MString& str, void* clientData) {

	MFnDependencyNode nodeFn(node);

	MString nameStr = PLUGINNAME;
	nameStr += " - Node Renamed (";
	nameStr += node.apiTypeStr();
	nameStr += "): '";
	nameStr += str;
	nameStr += "' -> '";
	nameStr += nodeFn.name();
	nameStr += "'";

	MGlobal::displayInfo(nameStr);

}

void polyTopologyChanged(MObject& node, void* clientData) {

	MFnMesh mesh(node);
	MItMeshVertex vertIter(node, &status);

	MString str = PLUGINNAME;
	str += " - PolyTopologyChanged (";
	str += node.apiTypeStr();
	str += ") ";
	str += mesh.name();
	str += " ";

	for (; !vertIter.isDone(); vertIter.next())
	{
		str += vertIter.index();
		str += " ";
	}
	MGlobal::displayInfo(str);
}

void timerFn(float elapsedTime, float lastTime, void* clientData) {

	MString str = PLUGINNAME;
	str += " - Time (";
	str += elapsedTime;
	str += ")";

	MGlobal::displayInfo(str);
};

void attributeChangedFn(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	if (msg & (MNodeMessage::kAttributeSet + MNodeMessage::kIncomingDirection))
	{
		if (plug.attribute().apiType() == MFn::kAttribute3Float)
		{
			// POINT CALLBACK
			MFnMesh mesh(plug.node(), &status);

			MPoint pnt;
			status = mesh.getPoint(plug.logicalIndex(), pnt);

			MString str = PLUGINNAME;
			str += " - AttributeChange (";
			str += plug.node().apiTypeStr();
			str += "): ";
			str += plug.name();
			str += " - ";

			if (status == MS::kSuccess)
			{
				str += "(";
				str += pnt.x;
				str += ",";
				str += pnt.y;
				str += ",";
				str += pnt.z;
				str += ")";

				MGlobal::displayInfo(str);
			}
			else
			{
				//MGlobal::displayInfo(PLUGINNAME + MString(" Point Error: ") + ret.errorString());
			}
		}
		else
		{
			//MGlobal::displayInfo(PLUGINNAME + MString(" Mesh Error: ") + ret.errorString());
		}

		if (plug.node().apiType() == MFn::kTransform)
		{
			// TRANSFORM CALLBACK

			MFnDagNode dag(plug.node(), &status);

			MDagPath path;
			dag.getPath(path);

			MObjectArray oa;

			MString str1;

			str1 += "CHILDCOUNT ";
			str1 += path.childCount();

			MGlobal::displayInfo(str1);

			while (path.length() > 0)
			{
				oa.append(path.node());

				MFnTransform transform(path.node(), &status);
				MTransformationMatrix worldMat(path.inclusiveMatrix());

				MString str = PLUGINNAME;
				str += " - AttributeChange (";
				str += path.node().apiTypeStr();
				str += "): ";
				str += path.fullPathName();
				str += " - ";

				if (plug.info().indexW("translate") > -1)
				{
					MVector localTranslate;
					localTranslate = transform.getTranslation(MSpace::kTransform);

					str += "translate ";
					str += "local (";
					str += localTranslate.x;
					str += ", ";
					str += localTranslate.y;
					str += ", ";
					str += localTranslate.z;
					str += ") ";

					MVector worldTranslate;
					worldTranslate = worldMat.translation(MSpace::kTransform);

					str += "world (";
					str += worldTranslate.x;
					str += ", ";
					str += worldTranslate.y;
					str += ", ";
					str += worldTranslate.z;
					str += ") ";

					MGlobal::displayInfo(str);
				}

				if (plug.info().indexW("scale") > -1)
				{
					double localScale[3];
					transform.getScale(localScale);

					str += "scale ";
					str += "local (";
					str += localScale[0];
					str += ", ";
					str += localScale[1];
					str += ", ";
					str += localScale[2];
					str += ") ";

					double worldScale[3];
					worldMat.getScale(worldScale, MSpace::kTransform);

					str += "world (";
					str += worldScale[0];
					str += ", ";
					str += worldScale[1];
					str += ", ";
					str += worldScale[2];
					str += ") ";

					MGlobal::displayInfo(str);
				}

				if (plug.info().indexW("rotate") > -1)
				{
					MEulerRotation localRotate;
					transform.getRotation(localRotate);

					str += "rotate ";
					str += "local (";
					str += localRotate.x * (180 / M_PI);
					str += ", ";
					str += localRotate.y * (180 / M_PI);
					str += ", ";
					str += localRotate.z * (180 / M_PI);
					str += ") ";

					MEulerRotation worldRotate;
					worldRotate = worldMat.eulerRotation();

					str += "world (";
					str += worldRotate.x * (180 / M_PI);
					str += ", ";
					str += worldRotate.y * (180 / M_PI);
					str += ", ";
					str += worldRotate.z * (180 / M_PI);
					str += ") ";

					MGlobal::displayInfo(str);
				}

				path.pop(1);
			}
		}
	}

	if (msg & (MNodeMessage::kAttributeEval + MNodeMessage::kIncomingDirection)) {

		if (plug.info().indexW("outMesh") > -1)
		{
			MItMeshVertex vertIter(plug.node(), &status);

			if (status == MS::kSuccess)
			{

				MString str = PLUGINNAME;
				str += " - AttributeChange (";
				str += plug.node().apiTypeStr();
				str += ") ";
				str += plug.name();
				str += " ";
				str += vertIter.className();
				str += " ";

				for (; !vertIter.isDone(); vertIter.next())
				{
					str += vertIter.index();
					str += " ";
				}

				MGlobal::displayInfo(str);
			}
		}
	}

	MString nodeMsgStr = PLUGINNAME;
	nodeMsgStr += " - AttributeChange (";
	nodeMsgStr += plug.node().apiTypeStr();
	nodeMsgStr += "): ";
	nodeMsgStr += plug.name();
	nodeMsgStr += " - ";

	if (msg & MNodeMessage::kConnectionMade)
	{
		nodeMsgStr += " kConnectionMade";
	}
	else if (msg & MNodeMessage::kConnectionBroken)
	{
		nodeMsgStr += " kConnectionBroken";
	}

	if (msg & MNodeMessage::kAttributeEval)
	{
		nodeMsgStr += " kAttributeEval";
	}

	if (msg & MNodeMessage::kAttributeSet)
	{
		nodeMsgStr += " kAttributeSet";
	}

	if (msg & MNodeMessage::kAttributeLocked)
	{
		nodeMsgStr += " kAttributeLocked";
	}
	else if (msg & MNodeMessage::kAttributeUnlocked)
	{
		nodeMsgStr += " kAttributeUnlocked";
	}

	if (msg & MNodeMessage::kAttributeAdded)
	{
		nodeMsgStr += " kAttributeAdded";
	}
	else if (msg & MNodeMessage::kAttributeRemoved)
	{
		nodeMsgStr += " kAttributeRemoved";
	}

	if (msg & MNodeMessage::kAttributeRenamed)
	{
		nodeMsgStr += " kAttributeRenamed";
	}

	if (msg & MNodeMessage::kAttributeKeyable)
	{
		nodeMsgStr += " kAttributeKeyable";
	}
	else if (msg & MNodeMessage::kAttributeUnkeyable)
	{
		nodeMsgStr += " kAttributeUnkeyable";
	}

	if (msg & MNodeMessage::kIncomingDirection)
	{
		nodeMsgStr += " kIncomingDirection";
	}

	if (msg & MNodeMessage::kAttributeArrayAdded)
	{
		nodeMsgStr += " kAttributeArrayAdded";
	}
	else if (msg & MNodeMessage::kAttributeArrayRemoved)
	{
		nodeMsgStr += " kAttributeArrayRemoved";
	}

	if (msg & MNodeMessage::kOtherPlugSet)
	{
		nodeMsgStr += " kOtherPlugSet";
		nodeMsgStr += " ";
		nodeMsgStr += otherPlug.info();
	}

	if (msg & MNodeMessage::kLast)
	{
		nodeMsgStr += " kLast";
	}

	//MGlobal::displayInfo(nodeMsgStr);
}

void appendCallbackFn(string name, MCallbackId* id, MStatus* status) {

	MString str = PLUGINNAME;
	str += " - ";
	str += name.c_str();
	str += ":";

	if (*status == MStatus::kSuccess)
	{
		str += " Success ";
		callbackIdArray.append(*id);
	}
	else
	{
		str += " Failed ";
	}

	MGlobal::displayInfo(str);
}

void addCallbacksFn()
{
	callbackId = MDGMessage::addNodeAddedCallback(nodeAddedFn, kDefaultNodeType, NULL, &status);
	appendCallbackFn("addNodeAddedCallback", &callbackId, &status);

	callbackId = MDGMessage::addNodeRemovedCallback(nodeRemovedFn, kDefaultNodeType, NULL, &status);
	appendCallbackFn("addNodeRemovedCallback", &callbackId, &status);

	callbackId = MTimerMessage::addTimerCallback(5.0, timerFn, NULL, &status);
	appendCallbackFn("addTimerCallback", &callbackId, &status);
}

/*
 * Plugin entry point
 * For remote control of maya
 * open command port: commandPort -nr -name ":1234"
 * close command port: commandPort -cl -name ":1234"
 * send command: see loadPlugin.py and unloadPlugin.py
 */
EXPORT MStatus initializePlugin(MObject obj) {

	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &status);

	if (MFAIL(status)) {
		CHECK_MSTATUS(status);
		return status;
	}

	MGlobal::displayInfo(PLUGINNAME + MString(" Plugin initialize"));

	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	std::cout.set_rdbuf(MStreamUtils::stdOutStream().rdbuf());
	std::cerr.set_rdbuf(MStreamUtils::stdErrorStream().rdbuf());

	// register callbacks here
	addCallbacksFn();

	// a handy timer, courtesy of Maya
	gTimer.clear();
	gTimer.beginTimer();

	return status;
}


EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);

	MGlobal::displayInfo(PLUGINNAME + MString(" Plugin uninitialize"));

	MMessage::removeCallbacks(callbackIdArray);

	return MS::kSuccess;
}