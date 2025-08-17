static CArchiveFactory gArchiveFactory;

bool CArchiveFactory::register_Internal(std::pair<const std::string&, constructor_t> inPair) {
	gArchiveFactory.m_Classes.insert(inPair);
	return true;
}

std::shared_ptr<void> CArchiveFactory::construct(const std::string& inTypeName) {
	if (!gArchiveFactory.m_Classes.contains(inTypeName)) {
		errs("Could not construct class {}", inTypeName.c_str());
	}
	return gArchiveFactory.m_Classes[inTypeName]();
}