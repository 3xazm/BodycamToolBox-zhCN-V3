#pragma once
#include "LocalizationModel.h"

class LocalizationPage {
public:
	LocalizationPage( ) = default;
	~LocalizationPage( ) = default;

	void Render( );

private:
	LocalizationModel m_Model;
};